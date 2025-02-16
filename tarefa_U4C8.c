// ---- Bibliotecas ----

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h" // Para PWM
#include "hardware/adc.h" // Para conversão ADC
#include "hardware/i2c.h" // Para interface de comunicação I2C
#include "inc/ssd1306.h" // Funções de configuração e manipulação com o display OLED
#include "inc/font.h" // Caracteres 8x8 para exibir no display OLED


// ---- Pinos, valores padrões e diretivas ----

// Macro para calcular o módulo de um valor. 
// Recebe um valor e retorna ele mesmo caso seja maior ou igual a 0, senão, retorna o oposto do valor
#define mod(x) ((x) >= 0 ? (x): -(x))

// LEDs
const uint8_t LED_GREEN_PIN = 11;
const uint8_t LED_BLUE_PIN = 12;
const uint8_t LED_RED_PIN = 13;
static volatile bool state_green_led = true;
static volatile bool state_blue_led = true;
static volatile bool state_red_led = true;

// Botões
const uint8_t BUTTON_A_PIN = 5;
// Armazena o último momento em que o botão foi pressionado
static volatile uint32_t last_time = 0; 

// PWM
const uint16_t PERIOD = 4096; // WRAP
const float DIVCLK = 16.0; // Divisor inteiro
static uint slice_13, slice_12; // Slices do GPIOS 12 e 13

// Joystick
const uint8_t VRx = 27;
const uint8_t VRy = 26;
const uint8_t ADC_CHAN_0 = 0;
const uint8_t ADC_CHAN_1 = 1; 
const uint8_t SW = 22; // Botão acoplado
const uint16_t half_adc = 2048; // Valor médio do joystick (estado de repouso)
const uint8_t death_zone = 250; // Garante um intervalo mínimo para ler os eixos do joystick

// I2C e Display OLED 1306
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
ssd1306_t ssd; // Inicializa a estrutura do display
static volatile bool display_edge = false; // Estado dos leds da borda do display


// ---- Inicializações ----

// Configura os canais de PWM dos LEDs vermelho e azul
void setup_pwm(){

    // PWM do LED VERMELHO
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    slice_13 = pwm_gpio_to_slice_num(LED_RED_PIN);
    pwm_set_clkdiv(slice_13, DIVCLK);
    pwm_set_wrap(slice_13, PERIOD);
    pwm_set_gpio_level(LED_RED_PIN, 0);
    pwm_set_enabled(slice_13, true);

    // PWM do LED AZUL
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);
    slice_12 = pwm_gpio_to_slice_num(LED_BLUE_PIN);
    pwm_set_clkdiv(slice_12, DIVCLK);
    pwm_set_wrap(slice_12, PERIOD);
    pwm_set_gpio_level(LED_BLUE_PIN, 0);
    pwm_set_enabled(slice_12, true);
    
}

// Inicializa o joystick e os conversores ADC
void init_joystick(){
    adc_init();
    adc_gpio_init(VRx);
    adc_gpio_init(VRy);

    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
}

// Obtém o valor atual das coordenadas do joystick com base na leitura ADC 
void read_joystick(uint16_t *vrx_value, uint16_t *vry_value){
    adc_select_input(ADC_CHAN_1);
    sleep_us(2);
    *vrx_value = adc_read();

    adc_select_input(ADC_CHAN_0);
    sleep_us(2);
    *vry_value = adc_read();
}

// Inicializa o LED verde
void init_led(){
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
}

// Inicializa o botão A
void init_button(){
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
}

// I2C
void initialize_i2c(){
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
}

// Recebe a coordenada do joystick e converte para a equivalente do display OLED 
void move_square(uint16_t vrx_value, uint16_t vry_value, uint8_t *x, uint8_t *y){
    *x = ((vrx_value * (128 - 8)) / 4095); // Posição x do quadrado no display
    *y = 56 - ((vry_value * (64 - 8)) / 4095); // Posição y do quadrado no display
}   


// ---- Interrupções ----

// Função callback de interrupção
void gpio_irq_handler(uint gpio, uint32_t events){
    
    // Guarda o tempo em us desde o boot do sistema
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if(current_time - last_time > 200) { // Efeito de debounce gerado pelo atraso de 200 ms na leitura do botão
        last_time = current_time;

         // Se o botão do joystick foi pressionado, os estados do LED verde e da borda do display OLED são alternados
        if(!gpio_get(SW)){
            state_green_led = !state_green_led;
            display_edge = !display_edge;
            gpio_put(LED_GREEN_PIN, state_green_led);
        }

        // Se o botão A foi pressonado, os estados dos LEDs vermelho e azul são alternados
        else if(!gpio_get(BUTTON_A_PIN)){
            state_red_led = !state_red_led;
            state_blue_led = !state_blue_led;
        }
    }            
}   


// ==== Programa Principal ====
int main()
{
    stdio_init_all(); // Função padrão de entrada/saída

    initialize_i2c();
    setup_pwm();
    init_led();
    init_button();
    init_joystick();


    // Configuração do Display OLED 1306
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Interrupção dos botões A e SW
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    uint16_t vrx_value, vry_value; // Valores ADC dos eixos do joystick
    uint8_t x = 0, y = 0; // Coordenadas do quadrado no display

    while (true) {

        // Limpa o display. Garante que o quadrado não apareça em dois lugares do display ao mesmo tempo
        ssd1306_fill(&ssd, false);
        ssd1306_send_data(&ssd);

        read_joystick(&vrx_value, &vry_value);

        // Atualiza a posição do quadrado no display e cria a borda retangular
        move_square(vrx_value, vry_value, &x, &y);
        ssd1306_square(&ssd, 8, x, y);
        ssd1306_rect(&ssd, 3, 3, 120, 56, display_edge, false);
        ssd1306_rect(&ssd, 3, 3, 121, 57, display_edge, false);    
        ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
        ssd1306_send_data(&ssd);

        // Se o LED vermelho estiver em true, e o eixo x estiver num intervalo de 250 pontos do repouso (2048)
        if(state_red_led && mod(vrx_value - half_adc) > death_zone){

            // O LED é regulado por PWM, baseado no valor do eixo x
            pwm_set_gpio_level(LED_RED_PIN, mod(vrx_value - half_adc));
        }

        // Senão, mantém o LED vermelho desligado
        else
            pwm_set_gpio_level(LED_RED_PIN, 0);

        // Se o LED azul estiver em true, e o eixo y estiver num intervalo de 250 pontos do repouso (2048)
        if(state_blue_led && mod(vry_value - half_adc) > death_zone){

             // O LED é regulado por PWM, baseado no valor do eixo y
            pwm_set_gpio_level(LED_BLUE_PIN, mod(vry_value - half_adc));
        }

        // Senão, mantém o LED azul desligado
        else
            pwm_set_gpio_level(LED_BLUE_PIN, 0);
            
        sleep_us(10);
    }
}
