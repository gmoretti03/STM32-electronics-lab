#include "stm32h7xx_hal.h"
#include "stm32h743xx.h"
#include "exponential.h"


uint16_t indice=500; // transmission index

uint32_t esponenziale_1[100], esponenziale_2[100]; // storage vectors
uint16_t stringa[100]; // difference of the two exponentials

uint32_t costante_1 = ((0.935506985)*(1<<10)); // decay constant (tau 1), fixed point
uint32_t costante_2 = ((0.904837418)*(1<<10)); // decay constant (tau 2), fixed point
uint8_t*stringa_p = (uint8_t *)stringa; // byte pointer used for transmission


void exp_USART_iniz(void){

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

// Computes the two exponentials with integer arithmetic
void exponential(void) {
	esponenziale_1[0]= (uint32_t)(65000); // first sample
	esponenziale_2[0] = (uint32_t)(65000); // first sample
	for(indice = 1 ;indice < 100; indice++) {
		esponenziale_1[indice]= (esponenziale_1[indice - 1]*costante_1 ) >> 10;
		esponenziale_2[indice]= (esponenziale_2[indice - 1]*costante_2 )>> 10;
		stringa[indice] = esponenziale_1[indice] - esponenziale_2[indice];
	}
}


void exp_USART_interrupt(void){
	// transmission handling
	if( USART3->ISR & USART_ISR_TC){
		if (indice < 200){ // 8-bit transmission of a 16-bit vector
			USART3 -> TDR = stringa_p[indice];
			indice++;
		}else{
			USART3->CR1 &= ~USART_CR1_TCIE; // disable transmit interrupt
			USART3->CR1 |= USART_CR1_RE; // enable receiver
			indice = 1;
		}
	}
	// reception handling
	if( USART3->ISR & USART_ISR_RXNE_RXFNE){
		if (USART3->RDR == 'w'){// 'w' is the request character sent by MATLAB
			USART3->CR1 &= ~USART_CR1_RE; // disable receiver
			exponential(); // compute
			USART3->CR1 |= USART_CR1_TCIE; // enable transmit interrupt
			USART3 -> TDR = stringa_p[0]; // send the first byte
			indice = 1;
		}
	}

	USART3->ICR |= USART_ICR_TCCF ;  // clear transmission-complete flag
	USART3->RQR |= USART_RQR_RXFRQ;  // flush the receive data
	USART3->ICR |= USART_ICR_ORECF; // clear overrun

}
