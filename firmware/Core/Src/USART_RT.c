#include "stm32h7xx_hal.h"
#include "USART_RT.h"

//**************************************//
// This file contains two implementations:
// the active one is a simple echo;
// the commented-out one replies to MATLAB with a fixed
// message once a command has been received.
//**************************************//

unsigned char ricezione[] = "abcde" ; // receive buffer
unsigned char trasmissione[] = "CiaoMat\r"; // message to send
unsigned char index_trasm = 0; // transmit index
unsigned char index = 0; // receive index

void RT_USART_iniz(void){
	// Enable transmitter and receiver
	 USART3 -> CR1 |= USART_CR1_RE;
	 USART3 -> CR1 |= USART_CR1_TE;
	 // Enable receive interrupt
	 USART3 -> CR1 |= USART_CR1_RXNEIE;
	 // Enable USART
	 USART3 -> CR1 |= USART_CR1_UE;

	 // reset USART flags
	 USART3->ICR |= USART_ICR_ORECF;// clear overrun flag
	 USART3->ICR |= USART_ICR_TCCF;// clear transmission-complete flag
	 USART3->RQR |= USART_RQR_RXFRQ;// flush the receive data
}

void RT_USART_interrupt(void){

	if(USART3->ISR & USART_ISR_RXNE_RXFNE){ // receive flag
		ricezione[index] =  USART3 -> RDR; // store the received byte
		if (ricezione[index] != '\r'){ // '\r' is the stop character
			index++;
		}
		else { // end of message: start transmitting
			USART3 -> CR1 |= USART_CR1_TCIE; // enable transmit interrupt
			// start transmission
			// echo
			index = 0;
			USART3 -> TDR = ricezione[index];
			index++;
			// fixed reply
//			USART3 -> TDR = trasmissione[index_trasm];
//			index_trasm++;
		}
	}
	// echo
	if(USART3->ISR & USART_ISR_TC){
		if (ricezione[index] != '\r'){
			USART3 -> TDR = ricezione[index];
			index++;
		}else{
		// last byte
			USART3 -> TDR = ricezione[index]; // send the end-of-message byte
			index = 0; // reset
			}
		}
	// fixed reply
//	if(USART3->ISR & USART_ISR_TC){
//			if (trasmissione[index_trasm] != '\r'){
//				USART3 -> TDR = trasmissione[index_trasm];
//				index_trasm++;
//			}else{
//			// last byte
//				USART3 -> TDR = trasmissione[index_trasm]; // send the end-of-message byte
//				index_trasm = 0; // reset
//				}
//			}


	USART3->ICR |= USART_ICR_ORECF; // clear overrun (happens when entering debug)
	USART3->ICR |= USART_ICR_TCCF;  // clear transmission-complete flag
	USART3->RQR |= USART_RQR_RXFRQ;  // flush the receive data
}
