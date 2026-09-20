#include "stm32f7xx.h"


int main(){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

	GPIOB->MODER &= ~(3 << (0*2));
	GPIOB->MODER |= (1 << (0*2));

	GPIOC->MODER &= ~(3 << (13*2));

	uint8_t last = 0;

	while(1){
		uint8_t now = (GPIOC->IDR >> 13) & 1;

		if (now == 1 && last == 0){
			GPIOB->ODR ^=(1 << 0);
		}
		last = now;

		for (volatile uint32_t i = 0; i < 200000; i++);
	}
}
