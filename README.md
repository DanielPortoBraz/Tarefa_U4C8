# Controle de PWM e Display com ADC na Raspberry Pi Pico W   

## **Descrição**  
O projeto a seguir, dispõe de um **joystick** que fornece valores analógicos dos seus eixos x e y, que controlam o **PWM** (modulção da largura de pulso) de um **LED RGB**. Nele, o **LED vermelho** varia conforme o **eixo x**, e o **LED azul** conforme o **eixo y**. Outra funcionalidade dos valores dos eixos, é o controle da posição de um **quadrado 8x8** num **display OLED** de **128x64 pixels** com comunicação **I2C**. Há também, **interrupções** controladas por **botões** para alternar estados dos LEDs e o estado de uma borda retangular exibida no display. No código, são utilizados os padrões do **PICO SDK C/C++** para execução na placa **Raspberry Pi Pico W**, enquanto os periféricos se baseiam nos disponíveis da placa **BitDogLab**.

## **Objetivos**  
• Compreender o funcionamento do conversor analógico-digital (ADC) no RP2040.

• Utilizar o PWM para controlar a intensidade de dois LEDs com base nos valores do joystick. 

• Representar a posição do joystick no display SSD1306 por meio de um quadrado móvel. 

• Aplicar o protocolo de comunicação I2C na integração com o display.

## **Componentes Utilizados**  
- Raspberry Pi Pico W  
- Joystick analógico (potenciômetros nos eixos X e Y)  
- Display OLED (SSD1306)  
- LEDs para controle de brilho via PWM    

## **Circuito e conexões (baseado na BitDogLab)**  

### **Joystick**  
| Pino do Joystick | Pino da Pico W | Função |  
|-----------------|---------------|--------|  
| VCC | 3V3 | Alimentação |  
| GND | GND | Terra |  
| VRX | GP27 (ADC1) | Eixo X |  
| VRY | GP26 (ADC0) | Eixo Y |  
| SW (Botão) | GP22 | Botão Pressionado |  

### **Display OLED**
| Pino do Display | Pino da Pico W | Função |  
|---------------|---------------|--------|  
| VCC | 3V3 | Alimentação |  
| GND | GND | Terra |  
| SDA | G14 | Dados I2C |  
| SCL | G15 | Clock I2C |  

### **LEDs**  
| Cor do LED | Pino da Pico W | Função |  
|------------|-------------|--------|  
| Vermelho | GP13 (PWM) | Controle PWM |  
| Azul | GP12 (PWM) | Controle PWM |  
| Verde | GP11 | Alternar estado |  

### Botão
| Pino da Pico W | Função |
|-------------|--------|
| GP5 (pull-up) | Alternar estado do LED verde |


## ADC na Raspberry Pi Pico W
O ADC (conversor analógico-digital) consiste na leitura de um valor analóigco que passa por um processo de conversão (na RP2040, a aproximação por amostra sucessiva (SAR)) que gera um valor digital equivalente. Nesse processo são utilizados dois conceitos: a amostragem, referente ao número de leituras feitas em determinado tempo; e a quantização, que converte o valor analógico ao digital de acordo com a resolução do conversor (i.e, a quantidade de valores representados em bits). Assim, o RP2040 permite conversões A/D, com uma taxa de amostragem de 500 ksps (amostras por segundo), e resolução de 12 bits (4096 valores). A seguir, a fórmula para conversão A/D com os valores do RP2040:

$$
D = \left\lfloor \frac{V_{in} \times 4095}{V_{ref}} \right\rfloor
$$

Onde:
- $( D $) é o valor digital convertido (entre **0** e **4095**);
- $( V_{in} $) é a tensão de entrada analógica (entre **0V** e **V_{ref}**);
- $( V_{ref} $) é a tensão de referência do ADC (típicamente **3.3V** no RP2040);
- $( 4095 $) representa o valor máximo em **12 bits** $((2^{12} - 1)$).

## Interação entre o ADC e o Display OLED
Os valores analógicos fornecidos pelos eixos **x** e **y** do joystick controlam o quadrado 8x8 no display OLED através da função ```move_square(uint16_t vrx_value, uint16_t vry_value, uint8_t *x, uint8_t *y)```, que calcula a posição do quadrado através de uma proporção aos valores analógicos dos eixos do joystick, para que no estado de repouso deste (aproximadamente, 2048 em cada eixo) o quadrado fique no meio do display. A proporção consiste em obter o valor analógico, calcular sua porcentagem com relação ao total da dimensão do display (no eixo **x** equivale a 128, por exemplo) e dividir o resultado por 4095. 

No caso da BitDogLab, o eixo **y** se comporta de maneira inversa, o que leva ao valor oposto daquele encontrado originalmente, por isso subtrai-se o resultado por 56. A implementação no código, nela deve-se observar que se subtrai 8 de cada eixo para garantir que o quadrado não ultrapasse o limite:

```c
void move_square(uint16_t vrx_value, uint16_t vry_value, uint8_t *x, uint8_t *y){
    *x = ((vrx_value * (128 - 8)) / 4095); // Posição x do quadrado no display
    *y = 56 - ((vry_value * (64 - 8)) / 4095); // Posição y do quadrado no display
} 
```

## Funções  
  -```setup_pwm()```: Configura os canais de PWM para os LEDs vermelho e azul, definindo as configurações de clock, período e nível inicial de sinal.  
  
  -```init_joystick()```: Inicializa os conversores ADC para leitura dos eixos do joystick e configura o botão acoplado como entrada com pull-up.  

  -```read_joystick(uint16_t *vrx_value, uint16_t *vry_value)```: Lê os valores analógicos dos eixos X e Y do joystick utilizando o ADC e armazena nos ponteiros fornecidos.  

  -```init_led()```: Inicializa o LED verde, configurando seu pino como saída.  

  -```init_button()```: Inicializa o botão A, configurando seu pino como entrada com pull-up.  

  -```initialize_i2c()```: Configura e inicializa a comunicação I2C, ativando as resistências de pull-up nos pinos de dados e clock.  
  -```move_square(uint16_t vrx_value, uint16_t vry_value, uint8_t *x, uint8_t *y)```: Converte os valores do joystick para coordenadas correspondentes no display OLED, garantindo um posicionamento proporcional do quadrado.  

  -```gpio_irq_handler(uint gpio, uint32_t events)```: Função de callback para interrupções nos botões. Alterna o estado dos LEDs e da borda do display OLED ao detectar um pressionamento.  

  -```main()```Função principal do programa. Inicializa os periféricos, configura interrupções e mantém um loop infinito para leitura do joystick, atualização do display OLED e controle dos LEDs via PWM.  


## **Instalação e Configuração**  
1. Instale o [SDK Pico C/C++](https://github.com/raspberrypi/pico-sdk).  
2. Clone este repositório e inicialize o SDK:  
   ```bash  
   git clone https://github.com/seuusuario/seurepositorio.git  
   cd seurepositorio  
   git submodule update --init  
   ```  
3. Compile o código com CMake:  
   ```bash  
   mkdir build && cd build  
   cmake ..  
   make  
   ```  
4. Grave o firmware na Pico W segurando **BOOTSEL** e copiando o `.uf2` para a unidade montada.
   
## Vídeo de demonstração



