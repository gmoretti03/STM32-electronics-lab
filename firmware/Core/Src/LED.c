#include "stm32h7xx_hal.h"
#include "LED.h"

// Variable that tracks which LED state the traffic light is in:
// led = 0: yellow
// led = 1: green
// led = 2: red
// led = 4: yellow blinking, currently off
// led = 5: yellow blinking, currently on
volatile int led = 1;

// Initial TIM6 setup
void LED_TIM6_iniz(void){
	TIM6 -> PSC = 5120;
	TIM6 -> ARR = 50000;
	TIM6 -> CR1 |= TIM_CR1_CEN;
	TIM6 -> DIER |= TIM_DIER_UIE;
}

// TIM6 interrupt handler
void LED_TIM6_interrupt(void){// traffic light implementation
	// yellow blinking
	if(led == 4){
		GPIOE->BSRR |= GPIO_BSRR_BS1;// yellow on
		led = 5;// state: yellow on
	}
	else if(led == 5) {
		GPIOE->BSRR |= GPIO_BSRR_BR1;// yellow off
		led = 4;// state: yellow off
	}

	// normal traffic light operation

	// red and green stay on longer than yellow
	else if(led == 1){ // green state
		GPIOE->BSRR |= GPIO_BSRR_BR1; // green off
		GPIOB->BSRR |= GPIO_BSRR_BS14;// red on
		TIM6 -> PSC = 5120; // timer frequency
		led = 2; // red state
	}
	else if(led == 0){ // yellow state
		GPIOB->BSRR |= GPIO_BSRR_BR0; // yellow off
		GPIOE->BSRR |= GPIO_BSRR_BS1;// green on
		led = 1; // green state
		TIM6 -> PSC = 5120; // timer frequency
	}
	else if(led == 2){ // red state
		GPIOB->BSRR |= GPIO_BSRR_BR14; // red off
		GPIOB->BSRR |= GPIO_BSRR_BS0; // yellow on
		led = 0; // yellow state
		TIM6 -> PSC = 2560; // timer frequency
	}
}

void LED_button_interrupt(void){
	// blinking yellow when the push-button is pressed
	if(led == 4 || led == 5) { // reset the traffic light
		// yellow LED off
		GPIOE->BSRR |= GPIO_BSRR_BR1;
		// lower the timer frequency
		TIM6 -> PSC = 5120;
		// back to green
		led = 1;
	}
	else { // start blinking
		// all LEDs off
		GPIOE->BSRR |= GPIO_BSRR_BR1;
		GPIOB->BSRR |= GPIO_BSRR_BR0;
		GPIOB->BSRR |= GPIO_BSRR_BR14;
		// raise the timer frequency
		TIM6 -> PSC = 1280;
		// state: yellow off
		led = 4;
	}
}
