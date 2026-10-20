#include "temp_volt.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_it.h"

// temperature sensor calibration values (factory-programmed)
#define TCAL_30C *(uint16_t *)(0x1FF1E820)
#define TCAL_110C *(uint16_t *)(0x1FF1E840)

uint8_t n = 1; // selects voltage (1) or temperature (0) sample, since temperature takes longer
uint8_t k = 0; // ADC sample counter
uint8_t t = 0; // temporary variable

// measurement vectors
uint16_t vect_volt[100]; // 16 bit
int32_t vect_temp[100]; // 32 bit

// byte pointers used for transmission
int8_t*temp_p = (int8_t *)vect_temp;
uint8_t*volt_p = (uint8_t *)vect_volt;

uint16_t trasm = 1000; // transmission counter

void temp_volt_ADC_iniz(void) {// ADC initialisation
	// select the input channels and the number of conversions
	// through the SQR1 register
	ADC3 -> SQR1 = 0;
	ADC3 -> SQR1 |= (19 << ADC_SQR1_SQ1_Pos) | (18 << ADC_SQR1_SQ1_Pos);
	ADC3 -> SQR1 |= (1 << ADC_SQR1_L_Pos);

	// PCSEL: select which channels are read
	ADC3 -> PCSEL |= ADC_PCSEL_PCSEL_19 | ADC_PCSEL_PCSEL_18;

	// calibration
	ADC3 -> CR &= ~ADC_CR_DEEPPWD_Pos; // deep power down off
	ADC3 -> CR |= (1 << ADC_CR_ADVREGEN_Pos); // enable voltage regulator

	ADC3 -> CR  &= ~ADC_CR_ADCALDIF_Pos; // single-ended measurements
	ADC3 -> CR |= (1 << ADC_CR_ADCALLIN_Pos); // linearity calibration
	ADC3 -> CR &= ~ADC_CR_ADEN_Pos; // ADC must be disabled to calibrate
	ADC3 -> CR |= (1 << ADC_CR_ADCAL_Pos); // start calibration
	while(ADC3 -> CR & ADC_CR_ADCAL){// wait for completion
	}

	// power-up
	ADC3 -> ISR  &= ~ADC_ISR_ADRDY_Pos; // clear ready bit
	ADC3 -> CR |= (1 << ADC_CR_ADEN_Pos); // enable ADC
	while(ADC3 -> ISR & ADC_ISR_ADRDY){ // wait until ready
	}
	ADC3 -> ISR  &= ~ ADC_ISR_ADRDY_Pos; // clear ready bit
	ADC3 -> IER |= ADC_IER_EOCIE; // end-of-conversion interrupt

	// sampling delay: longer for better precision, needed for temperature
	ADC3_COMMON -> CCR |= 0xb << ADC_CCR_PRESC_Pos;
	ADC3 -> SMPR2 |= (7 << ADC_SMPR2_SMP18_Pos);


	// To read temperature and voltage, the TSEN and VREFEN flags must be set in CCR
	ADC3_COMMON -> CCR |= ADC_CCR_TSEN;
	ADC3_COMMON -> CCR |= ADC_CCR_VREFEN;

	// CFGR.CONT selects single or continuous conversion.
	// A trigger (e.g. TIM6) could be used, or the software trigger
	// via ADSTART in CR, which is recommended to start with.
	ADC3 -> CFGR |= (1 << ADC_CFGR_CONT_Pos); // continuous conversion
	ADC3 -> CFGR |= ADC_CFGR_AUTDLY; // do not start a conversion while the previous result is unread
}// end of ADC initialisation

// ADC measurements
void temp_volt_ADC_interrupt(void){
	if(k < 100 && n == 1){
		vect_volt[k] = ADC3 -> DR; // rescaled to volts in MATLAB
		n = 0;
		k++;
	}
	else if(k < 101 && n == 0){
		t = ADC3 -> DR;
		// integer temperature conversion
		vect_temp[k - 1] = 30000 + (((t - TCAL_30C) * 80000)/(TCAL_110C - TCAL_30C)); // millidegrees C
		n = 1;
	}
	else{ // measurements finished
		ADC3 -> IER &= ~ADC_IER_EOCIE;
		USART3->CR1 |= USART_CR1_TCIE;
		USART3->CR1 |= USART_CR1_UE; // enable USART
		trasm = 1; // set index
		USART3 -> TDR = volt_p[0]; // start transmission
	}
}


void temp_volt_USART_interrupt(void){
	if( USART3->ISR & USART_ISR_TC){
		// transmission handling
		if (trasm < 200 && n == 1){
			USART3 -> TDR = volt_p[trasm];
			trasm++;
		}else if(trasm == 200 && n == 1){
			USART3 -> TDR = temp_p[0];
			trasm++;
			n = 0;
		// finish sending the temperature vector
		}else if(trasm > 200 && trasm < 600 && n == 0){
			USART3 -> TDR = temp_p[trasm - 200];
			trasm++;
		}else if(trasm == 600 && n == 0){
			USART3->CR1 &= ~USART_CR1_TCIE; // disable transmit interrupt
			USART3->CR1 |= USART_CR1_RE; // start receiving again
			trasm = 1000; // keep the USART from re-entering this branch when not requested
			n = 1; // ready for the next acquisition
		}
	}

	if(USART3->ISR & USART_ISR_RXNE_RXFNE){
		// reception handling
		if (USART3->RDR == 'w'){// 'w' is the request character sent by MATLAB
			USART3->CR1 &= ~USART_CR1_RE;// disable receiver while transmitting
			ADC3 -> CR |= ADC_CR_ADSTART;// start measurements
			}
	}

	USART3->ICR |= USART_ICR_TCCF ;  // clear transmission-complete flag
	USART3->RQR |= USART_RQR_RXFRQ;  // flush the receive data
	USART3->ICR |= USART_ICR_ORECF; // clear overrun

}



void temp_volt_USART_iniz(void){

	// Transmitter
	//USART3->CR1 |= USART_CR1_TE;
	// Enable receiver
	USART3->CR1 |= USART_CR1_RE;
	// Enable receive interrupt
	USART3->CR1 |=USART_CR1_RXNEIE ;
	// Enable transmit interrupt
	USART3->CR1 |=USART_CR1_TCIE ;
	// Enable USART
	USART3->CR1 |= USART_CR1_UE;

}
