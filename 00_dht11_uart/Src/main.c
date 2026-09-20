#include "stm32f7xx.h"

void delay_ms(volatile uint32_t ms) {
    for (uint32_t i = 0; i < ms * 3300; i++);
}

void GPIO_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    GPIOB->MODER &= ~(3U << (0 * 2));
    GPIOB->MODER |=  (1U << (0 * 2));
}

void TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 16 - 1;
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->EGR |= TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void UART3_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    GPIOD->MODER &= ~((3U << (8*2)) | (3U << (9*2)));
    GPIOD->MODER |=  ((2U << (8*2)) | (2U << (9*2)));

    GPIOD->AFR[1] &= ~((0xF << ((8-8)*4)) | (0xF << ((9-8)*4)));
    GPIOD->AFR[1] |=  ((7U << ((8-8)*4)) | (7U << ((9-8)*4)));

    USART3->BRR = 1667;
    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART3_SendChar(char c) {
    while (!(USART3->ISR & USART_ISR_TXE));
    USART3->TDR = c;
}

void UART3_SendString(char *str) {
    while (*str) UART3_SendChar(*str++);
}

void UART3_SendNumber(uint32_t num) {
    char buf[10];
    int i = 0;
    if (num == 0) { UART3_SendChar('0'); return; }
    while (num > 0) { buf[i++] = (num % 10) + '0'; num /= 10; }
    while (i > 0) UART3_SendChar(buf[--i]);
}

void DHT_SetOutput(void) {
    GPIOA->MODER &= ~(3U << (1 * 2));
    GPIOA->MODER |=  (1U << (1 * 2));
}

void DHT_SetInput(void) {
    GPIOA->MODER &= ~(3U << (1 * 2));
}

uint8_t DHT_ReadByte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        TIM2->CNT = 0;
        while (!(GPIOA->IDR & (1 << 1))) {
            if (TIM2->CNT > 200) break;
        }

        TIM2->CNT = 0;
        while (GPIOA->IDR & (1 << 1)) {
            if (TIM2->CNT > 200) break;
        }
        uint32_t high_time = TIM2->CNT;

        byte <<= 1;
        if (high_time > 40) {
            byte |= 1;
        }
    }
    return byte;
}

int DHT_Read(uint8_t *humidity, uint8_t *temperature) {
    uint8_t data[5] = {0,0,0,0,0};

    DHT_SetOutput();
    GPIOA->ODR &= ~(1 << 1);
    delay_ms(20);
    GPIOA->ODR |= (1 << 1);

    TIM2->CNT = 0;
    while (TIM2->CNT < 30);

    DHT_SetInput();

    TIM2->CNT = 0;
    while (GPIOA->IDR & (1 << 1)) {
        if (TIM2->CNT > 100) { UART3_SendString("T1\r\n"); return -1; }
    }

    TIM2->CNT = 0;
    while (!(GPIOA->IDR & (1 << 1))) {
        if (TIM2->CNT > 100) { UART3_SendString("T2\r\n"); return -1; }
    }

    TIM2->CNT = 0;
    while (GPIOA->IDR & (1 << 1)) {
        if (TIM2->CNT > 100) { UART3_SendString("T3\r\n"); return -1; }
    }

    for (int i = 0; i < 5; i++) {
        data[i] = DHT_ReadByte();
    }

    UART3_SendString("Raw: ");
    for (int i = 0; i < 5; i++) {
        UART3_SendNumber(data[i]);
        UART3_SendString(" ");
    }
    UART3_SendString("\r\n");

    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
        return -2;
    }

    *humidity = data[0];
    *temperature = data[2];
    return 0;
}

int main(void) {
    GPIO_Init();
    TIM2_Init();
    UART3_Init();

    while (1) {
        uint8_t hum, temp;
        int result = DHT_Read(&hum, &temp);

        if (result == 0) {
            UART3_SendString("Sicaklik: ");
            UART3_SendNumber(temp);
            UART3_SendString("C  Nem: ");
            UART3_SendNumber(hum);
            UART3_SendString("%\r\n");
            GPIOB->ODR |= (1 << 0);
        } else {
            UART3_SendString("Hata: ");
            UART3_SendNumber((uint32_t)(-result));
            UART3_SendString("\r\n");
            GPIOB->ODR &= ~(1 << 0);
        }

        delay_ms(2000);
    }
}
