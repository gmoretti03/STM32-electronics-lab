/*
 * ESP_ADC.c
 *
 *  Created on: Nov 6, 2023
 *      Author: utente
 */


#include "ESP_ADC_0.h"
#include <stm32h7xx.h> //lui definisce che cosa sono gli adc etc.

uint16_t voltaggio[10];
uint16_t temperatura[10];
int ii = -1;
int indice = 1;
int ciao = 0;

//inzializzazione ADC
void ESPE_ADC_0_init(void) {

	ADC3->SQR1 = 0; 
    ADC3->SQR1 |= 1<<ADC_SQR1_L_Pos; 
	ADC3->SQR1 |= (19<<ADC_SQR1_SQ1_Pos)|(18<<ADC_SQR1_SQ2_Pos);
	ADC3->PCSEL |= ADC_PCSEL_PCSEL_19 | ADC_PCSEL_PCSEL_18;
	ADC3 -> CR &= ~ADC_CR_DEEPPWD_Msk; 
	ADC3 -> CR |= ADC_CR_ADVREGEN_Msk; 
	ADC3 -> CR &= ~ADC_CR_ADCALDIF_Msk; 
	ADC3 -> CR |= ADC_CR_ADCALLIN_Msk; 
	ADC3 -> CR &= ~ADC_CR_ADEN_Msk; 
	ADC3 -> CR |= ADC_CR_ADCAL_Msk; 

	while (ADC3 -> CR & ADC_CR_ADCAL_Msk){}
	
	ADC3 -> ISR &= ~ADC_ISR_ADRDY_Msk; 
	ADC3 -> CR |= ADC_CR_ADEN_Msk;

	while (~(ADC3 -> ISR) & ADC_ISR_ADRDY) {}

	ADC3 -> ISR &= ~ADC_ISR_ADRDY_Msk; 
	ADC3 -> IER |= ADC_IER_EOCIE_Msk;
	
	ADC3 -> SMPR2 |= (7<<ADC_SMPR2_SMP18_Pos);
	ADC3 -> SMPR2 |= (7<<ADC_SMPR2_SMP19_Pos);

	ADC3_COMMON -> CCR |= ADC_CCR_TSEN_Msk | ADC_CCR_VREFEN_Msk;

	ADC3 -> CFGR &= ~ADC_CFGR_CONT_Msk;

	ADC3 -> CFGR &= ~ADC_CFGR_EXTEN_Msk;

	ADC3 -> CFGR |= ADC_CFGR_AUTDLY_Msk; 
}

void MISURA (void){ 
	ADC3 -> CR |= ADC_CR_ADSTART_Msk; 
}

int contatore (void) {

	if (~(ADC3->ISR)  & ADC_ISR_EOC){
		ii++;
	}

	return ii;

}

//inizializzazione dell'USART
void inizializzo_USART(void){
	USART3->CR1 |= USART_CR1_TE;
	USART3->CR1 |= USART_CR1_RE;
	USART3->CR1 |=USART_CR1_RXNEIE ;
	USART3->CR1 |=USART_CR1_TCIE ;
	USART3->CR1 |= USART_CR1_UE;
}


//inizializzo e configuro l'interrupt di usart 
void ESPE_USART_interrupt(void){
	
	uint8_t* volt_char = (uint8_t*)(voltaggio);

	if( USART3->ISR & USART_ISR_TC){
		if (indice < 20 && ciao){
			USART3 -> TDR = volt_char[indice];
			indice ++;
		}
		else {
			USART3->CR1 &= ~USART_CR1_TCIE ;
		}
	}

	if( USART3->ISR & USART_ISR_RXNE_RXFNE){
		
		if (USART3->RDR == 'w'){
			USART3->CR1 &= ~USART_CR1_RE; 
		}
	}

	USART3->ICR |= USART_ICR_TCCF ; 
	USART3->RQR |= USART_RQR_RXFRQ;  
	USART3->ICR |= USART_ICR_ORECF; 
}

