#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

// Pinos do Joystick
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_PB 22
#define BOTAO_A 5

// Pinos dos LEDs RGB
#define LED_R 11
#define LED_G 12
#define LED_B 13

bool estado_led_verde = false;
bool estado_led_pwm = true;
bool alterar_borda = false;

// Estrutura do Display
ssd1306_t ssd;

void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == JOYSTICK_PB) {
        estado_led_verde = !estado_led_verde;
        alterar_borda = !alterar_borda;
    }
    if (gpio == BOTAO_A) {
        estado_led_pwm = !estado_led_pwm;
    }
}

void configurar_pwm(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 4095);
    pwm_set_enabled(slice, true);
}

void atualizar_display(uint16_t x, uint16_t y) {
  char str_x[5], str_y[5];
  sprintf(str_x, "%d", x);
  sprintf(str_y, "%d", y);

  // Converte o ADC (0-4095) para coordenadas da tela (0-127, 0-63)
  int pos_x = (x * 64) / 4095;  
  int pos_y = (y * 128) / 4095;    

  // Garante que o quadrado não ultrapasse os limites da tela
  if (pos_x > 55) pos_x = 55; // Limite para manter o quadrado dentro da borda
  if (pos_y > 119) pos_y = 119;

  ssd1306_fill(&ssd, false);
  ssd1306_rect(&ssd, 3, 4, 122, 60, true, alterar_borda);
  ssd1306_rect(&ssd, pos_x, pos_y, 8, 8, true, true);
  ssd1306_send_data(&ssd);
}


int main() {
    stdio_init_all();
    
    // Inicializa os botões com pull-up
    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);
    gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa I2C e display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, 128, 64, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Configura PWM para LEDs
    configurar_pwm(LED_R);
    configurar_pwm(LED_G);
    configurar_pwm(LED_B);

    while (true) {
        adc_select_input(0);
        uint16_t adc_value_x = adc_read();
        adc_select_input(1);
        uint16_t adc_value_y = adc_read();

        if (estado_led_pwm) {
            pwm_set_gpio_level(LED_B, 4095 - adc_value_y);
            pwm_set_gpio_level(LED_R, 4095 - adc_value_x);
        } else {
            pwm_set_gpio_level(LED_B, 0);
            pwm_set_gpio_level(LED_R, 0);
        }

        gpio_put(LED_G, estado_led_verde);
        atualizar_display(adc_value_x, adc_value_y);
        sleep_ms(100);
    }
}
