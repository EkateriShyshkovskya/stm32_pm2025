#include <stdint.h>
#include <stm32f10x.h>

// Переменные для управления частотой
volatile uint32_t blink_frequency = 1; // 1 Гц по умолчанию
volatile uint32_t blink_delay = 1000000; // Задержка для 1 Гц

void delay(uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

// Функция для чтения состояния кнопок
uint8_t read_button_a(void) {
    return (GPIOD->IDR & GPIO_IDR_IDR6) == 0; // PD6 - кнопка A
}

uint8_t read_button_b(void) {
    return (GPIOD->IDR & GPIO_IDR_IDR7) == 0; // PD7 - кнопка B
}

// Функция обновления задержки на основе частоты
void update_delay(void) {
    blink_delay = 1000000 / blink_frequency;
}

// Функция обработки нажатий кнопок
void process_buttons(void) {
    static uint32_t last_check = 0;
    static uint32_t counter = 0;
    
    counter++;
    if (counter - last_check < 100000) {
        return;
    }
    last_check = counter;
    
    // Кнопка A работает только если частота меньше 64 Гц
    if (read_button_a() && blink_frequency < 64) {
        blink_frequency *= 2;
        update_delay();
    }
    
    // Кнопка B работает только если частота больше 1/64 Гц  
    if (read_button_b() && blink_frequency > 1) {
        blink_frequency /= 2;
        update_delay();
    }
}

int __attribute__((noreturn)) main(void) {
    // Enable clock for AFIO
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    // Enable clock for GPIOC (светодиод)
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    // Enable clock for GPIOD (кнопки)
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    
    // Configure PC13 as push-pull output (светодиод)
    GPIOC->CRH &= ~GPIO_CRH_CNF13; // clear CNF bits
    GPIOC->CRH |= GPIO_CRH_MODE13_0; // Max speed = 10MHz
    
    // Configure PD6 and PD7 as input with pull-up (кнопки)
    GPIOD->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_CNF7); // clear CNF bits
    GPIOD->CRL |= (GPIO_CRL_CNF6_1 | GPIO_CRL_CNF7_1); // Input with pull-up/pull-down
    GPIOD->BSRR = GPIO_BSRR_BS6 | GPIO_BSRR_BS7; // Set pull-up
    
    // Отключаем JTAG чтобы освободить PD pins
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_1; // JTAG-DP Disabled, SW-DP Enabled

    while (1) {
        // Обрабатываем нажатия кнопок
        process_buttons();
        
        // Переключаем светодиод
        GPIOC->ODR |= (1U << 13U); // Включить
        delay(blink_delay);
        GPIOC->ODR &= ~(1U << 13U); // Выключить
        delay(blink_delay);
    }
}
