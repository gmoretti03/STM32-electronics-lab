
#include "SiPM.h"
#include "stm32h7xx_hal.h"


// General note:
// this code acquires a vector of measurement sets ("DMA_ADC_vettore").
// Each set holds "numero_misure_ADC" samples and is followed by a value
// ("NDTR") needed to reconstruct the waveform in MATLAB.
// For the SiPM experiment, MATLAB requests 10000 waveforms from the microcontroller.
// The code could instead have been changed so that MATLAB requests a single vector
// containing all the measurements.

volatile int NN = 0; // number of samples after the trigger
volatile int TT = 0; // trigger state
volatile int numero_misure_ADC = 100;// samples per measurement set
volatile int dimensione_misure = 1; // number of measurement sets
volatile uint16_t DMA_ADC_vettore[(100 + 1)*1]; // vector of measurement sets
volatile int DD = 0; // measurement-set counter

void SiPM_ADC_iniz(void) {// ADC initialisation
// select the input channels and the number of channels through the SQR1 register
	ADC3 -> SQR1 = 0;
	ADC3 -> SQR1 |= (0 << ADC_SQR1_SQ1_Pos);
	ADC3 -> SQR1 |= (0 << ADC_SQR1_L_Pos);

// PCSEL: select which channels are read
	ADC3 -> PCSEL |= ADC_PCSEL_PCSEL_0;
	// calibration
	ADC3 -> CR &= ~ADC_CR_DEEPPWD_Pos; // deep power down off
	ADC3 -> CR |= (1 << ADC_CR_ADVREGEN_Pos);// enable voltage regulator

	ADC3 -> CR  &= ~ADC_CR_ADCALDIF_Pos;// single-ended measurements
	ADC3 -> CR |= (1 << ADC_CR_ADCALLIN_Pos); // linearity calibration
	ADC3 -> CR &= ~ADC_CR_ADEN_Pos;// disable ADC
	ADC3 -> CR |= (1 << ADC_CR_ADCAL_Pos);// start calibration
	while(ADC3 -> CR & ADC_CR_ADCAL){// wait for completion
	}

	ADC3 -> ISR  &= ~ADC_ISR_ADRDY_Pos;// clear bit
	ADC3 -> CR |= (1 << ADC_CR_ADEN_Pos);// enable ADC
	while(ADC3 -> ISR & ADC_ISR_ADRDY){// wait until ready
	}
	ADC3 -> ISR  &= ~ ADC_ISR_ADRDY_Pos;// clear bit
	ADC3 -> IER |= ADC_IER_EOCIE;// end-of-conversion interrupt

	// non-circular DMA

	//ADC3->CFGR |= (1<< ADC_CFGR_DMNGT_Pos);

	// circular DMA
	ADC3->CFGR |= (3<< ADC_CFGR_DMNGT_Pos);
}


void SiPM_DMA_interrupt(void){
	if (!(DMA1-> LISR & DMA_LIFCR_CTCIF3)) // check the transfer-complete flag
		return;
	// reset flag registers
	DMA1-> LIFCR = 0xffffffff;
	DMA1-> HIFCR = 0xffffffff;

	// discrete measurements
	DMA1_Stream3->CR &= ~DMA_SxCR_EN; // disable DMA
	USART3->CR1 |= USART_CR1_RE; // enable receiver

	// continuous measurements

	//DMA1_Stream3->CR &= ~DMA_SxCR_EN;
	//DMA2_Stream0->CR |= DMA_SxCR_EN;
	//ADC3 -> CR |= ADC_CR_ADSTART;
	//TIM6 -> CR1 |= TIM_CR1_CEN;
}

// non-circular DMA

//void DMA2_interrupt(void){
//	TIM6 -> CR1 &= ~TIM_CR1_CEN;
//	DMA2-> LIFCR = 0xffffffff;
//	DMA2-> HIFCR = 0xffffffff;
//
//	DMA2_Stream0->CR &= ~DMA_SxCR_EN;
//	DMA1_Stream3->CR |= DMA_SxCR_EN;
//
//}

void SiPM_ADC_interrupt(void){

	// digital trigger

//	data_pre = data_post;
//	data_post = ADC3 -> DR;
//	if(data_post > 30000 && data_pre > data_post){
//		N = 1;
//	}
//	if(N != 0){
//		N++;
//	}
//	if(N == 250){
//		data_post = 0;
//		data_pre = 0;
//		N = 0;
//		DMA_ADC_vettore[numero_misure_ADC] = DMA2_Stream0 -> NDTR;
//		TIM6 -> CR1 &= ~TIM_CR1_CEN;
//		DMA2-> LIFCR = 0xffffffff;
//		DMA2-> HIFCR = 0xffffffff;
//		DMA2_Stream0->CR &= ~DMA_SxCR_EN;
//		DMA1_Stream3->CR |= DMA_SxCR_EN;
//	}


	// analog trigger (comparator)

	if(TT ==0){// waiting for the trigger
		if(COMP12->SR&COMP_SR_C1VAL){
			TT = 1;
			NN = 0;
		}
	}
	else{
		NN++;
	}
	if(NN == 90){// post-trigger samples collected
		TT = 0;
		NN = 0;
		// save the circular-DMA index
		DMA_ADC_vettore[(numero_misure_ADC*dimensione_misure) + DD] = DMA2_Stream0 -> NDTR;
		DD++;
		if(DD == 1){
			DD = 0;
			TIM6 -> CR1 &= ~TIM_CR1_CEN; // stop timer
			// reset flag registers
			DMA2-> LIFCR = 0xffffffff;
			DMA2-> HIFCR = 0xffffffff;
			DMA2_Stream0->CR &= ~DMA_SxCR_EN; // disable DMA

			// reset flag registers
			DMA1-> LIFCR = 0xffffffff;
			DMA1-> HIFCR = 0xffffffff;
			// set up the DMA transfer that sends the collected set to the USART
			DMA1_Stream3->M0AR = (uint32_t)&DMA_ADC_vettore;
			DMA1_Stream3 -> PAR = (uint32_t)(&USART3->TDR);
			DMA1_Stream3-> NDTR = (numero_misure_ADC*dimensione_misure + 1)*2;
			DMA1_Stream3->CR |= DMA_SxCR_EN;
		}
		else{
			DMA2_Stream0->CR &= ~DMA_SxCR_EN;
			// point the ADC DMA at the next measurement set
			DMA2_Stream0->M0AR = (uint32_t)&DMA_ADC_vettore[numero_misure_ADC*DD];
			DMA2_Stream0->CR |= DMA_SxCR_EN;
		}
	}
}

void SiPM_USART_interrupt(void){
	// receive-only handling
	if( USART3->ISR & USART_ISR_RXNE_RXFNE){
		if (USART3->RDR == 'w'){// 'w' is the request character sent by MATLAB
			USART3->CR1 &= ~USART_CR1_RE;
			DMA2_Stream0->CR |= DMA_SxCR_EN;
			ADC3 -> CR |= ADC_CR_ADSTART;
			TIM6 -> CR1 |= TIM_CR1_CEN;
		}
	}

	USART3->ICR |= USART_ICR_TCCF ;  // clear transmission-complete flag
	USART3->RQR |= USART_RQR_RXFRQ;  // flush the receive data
	USART3->ICR |= USART_ICR_ORECF; // clear overrun

}

void SiPM_comp_iniz(void){
	COMP1->CFGR |= COMP_CFGRx_EN; // enable comparator

	DAC1->CR|=DAC_CR_EN1; // enable DAC
	DAC1->DHR12R1 = 1240; // 12-bit value, full scale 3.3 V (comparator threshold)
	DAC1->SWTRIGR |= DAC_SWTRIGR_SWTRIG1; // update DAC output
}



void SiPM_USART_iniz(void){
	USART3->CR1 |= USART_CR1_UE;

	// Transmitter
	//USART3->CR1 |= USART_CR1_TE;
	// Enable receiver
	USART3->CR1 |= USART_CR1_RE;
	// Enable receive interrupt
	USART3->CR1 |=USART_CR1_RXNEIE ;
	// DMA transmit requests
	USART3->CR3 |= USART_CR3_DMAT;

}

void SiPM_DMA_iniz(void){
	// point the DMA streams at the buffer
	DMA2_Stream0->M0AR = (uint32_t)&DMA_ADC_vettore;
	DMA1_Stream3->M0AR = (uint32_t)&DMA_ADC_vettore;
	// set the peripheral addresses
	DMA2_Stream0 -> PAR = (uint32_t)(&ADC3->DR);
	DMA1_Stream3 -> PAR = (uint32_t)(&USART3->TDR);
	// set the NDTR counter for the ADC
	DMA2_Stream0 -> NDTR = numero_misure_ADC;
	// non-circular DMA
	//DMA1_Stream3-> NDTR = numero_misure_ADC * 2;
	//DMA2_Stream0->CR |= DMA_SxCR_TCIE;

	// circular DMA
	// set NDTR for the USART
	DMA1_Stream3-> NDTR = (numero_misure_ADC*dimensione_misure + 1)*2;
	DMA1_Stream3->CR |= DMA_SxCR_TCIE;

}

void SiPM_timer_iniz(void){
	// maximum value is 65535 (16 bit)
	// CLOCK/PSC = ARR with CLOCK = 240 MHz
	// with PSC = 64 and ARR = 62500 the timer period is 1 second
	// multiplying PSC divides the frequency by that factor
	/*
	TIM6 -> CNT = 0;
	TIM6 -> PSC = 64*100; // timer period 10 ms
	TIM6 -> ARR = 62500;
	*/

	// sampling timer: PSC = 0, ARR = 240
	TIM6 -> CNT = 0;
	TIM6 -> PSC = 0;
	TIM6 -> ARR = 240;

}






