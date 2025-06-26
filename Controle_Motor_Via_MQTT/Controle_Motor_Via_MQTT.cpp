// Lista 3 de exercicio IOT Eletrônica embarcada 2025

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "bsp/board.h"
#include <string.h>
#include "hardware/pwm.h"

#define EN_A 18
#define A1 16
#define A2 17

//Raspberry Pico 2W
#define TOP 29999

char Tp_Dados [30] = "TESTE";// Vetor que irá receber o tipo de dados
volatile char valor = 0;
volatile char flag1 = 0;
volatile char flag2 = 0;
volatile char flag3 = 0; 
volatile float flag4 = 0;
volatile char flag5 = 0;


// SH indica rotação no sentido hoário 
// AH indica sentido anti hoário
// + indica aumento da intensidade do PWM
// - indica diminuição da intensidade do PWM


//Raspberry Pico W
//#define TOP 24999

void init_io_mode();
void init_pwm_mode();
void set_duty_cycle_prop(float dut_prop);
void op_pwm_mode();

void init_io_mode() {
    gpio_init(EN_A);
    gpio_init(A1);
    gpio_init(A2);

    gpio_set_dir(EN_A, GPIO_OUT);
    gpio_set_dir(A1, GPIO_OUT);
    gpio_set_dir(A2, GPIO_OUT);

    gpio_put(EN_A, false);
    gpio_put(A1, false);
    gpio_put(A2, false);
}

void init_pwm_mode()
{
    gpio_init(EN_A);
    gpio_set_function(EN_A, GPIO_FUNC_PWM);
    int fatia_pwm = pwm_gpio_to_slice_num(EN_A);
    pwm_set_wrap(fatia_pwm, TOP);
    pwm_set_clkdiv(fatia_pwm, 1.0f);
    pwm_set_enabled(fatia_pwm, true);

    gpio_init(A1);
    gpio_init(A2);

    gpio_set_dir(A1, GPIO_OUT);
    gpio_set_dir(A2, GPIO_OUT);

    gpio_put(A1, false);
    gpio_put(A2, false);
    pwm_set_gpio_level(EN_A, 0); 
}

void set_duty_cycle_prop(float dut_prop) 
{
    int slice = pwm_gpio_to_slice_num(EN_A);
    int duty_cycle = (int) (dut_prop * TOP);
    pwm_set_gpio_level(EN_A, duty_cycle);
}
/*
 * Estrutura para armazenar as informações do cliente MQTT
 * identificador, usuário e senha...
 */
struct mqtt_connect_client_info_t info_cliente=
{
    "Anderson Camargo", /* identificador do cliente */
    NULL, /* Usuário */
    NULL, /*Senha*/
    0, /*keep alive*/
    NULL, /*Tópico do último desejo*/
    NULL, /*Mensagem do Último Desejo*/
    0, /*Qualidade do Serviço do Último Desejo*/
    0 /*Último Desejo retentivo*/
};

static void mqtt_dados_recebidos_cb(void *arg, const u8_t *dados, u16_t comprimento, u8_t flags)
{
   u16_t tam = comprimento < sizeof(Tp_Dados) - 1 ? comprimento : sizeof(Tp_Dados) - 1;
   strncpy(Tp_Dados, (const char *)dados, tam);
   Tp_Dados[tam] = '\0'; // Garante que Tp_Dados seja uma string válida;
    
    if(Tp_Dados[0] == 'S' && Tp_Dados[1] == 'H' )
    {
       valor = 1; // Condição que indica atuação no sentido horário
       flag1 = 1; // Sinaliza que o motor está girando no sentido horário
           
    }
    else if(Tp_Dados[0] == 'A' && Tp_Dados[1] == 'H')
    {
       valor = 2; // Condição que indica atuação no sentido anti-horário
       flag2 = 1; 
       
    }
    else if(Tp_Dados[0] == '+')
    {
        valor = 3; // Condição para aumentar a intensidade do PWM
    }
    else if(Tp_Dados[0] == '-')
    {
        valor = 4; // Condição para diminuir a intensidade do PWM
    }
    sleep_ms(100); // Aguarda 1 segundo para garantir que a string foi copiada corretamente
}

static void mqtt_chegando_publicacao_cb(void *arg, const char *topico, u32_t tamanho){

}

static void mqtt_req_cb(void *arg, err_t erro) {

}


static void mqtt_conectado_cb(mqtt_client_t *cliente, void *arg, mqtt_connection_status_t status) {

    if(status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao Broker MQTT com sucesso!\n");

       mqtt_subscribe(cliente, "pwm", 0, &mqtt_req_cb, NULL);

    } else {
        printf("Falha na conexão com o Broker MQTT: %d\n", status);
    }
}

void op_pwm_mode() {

    static float duty_cycle = 0;
    flag4 = duty_cycle;
    
    if(valor == 1  &&   flag2 == 1)
    {
       gpio_put(EN_A, false);
       flag2 = 0; 
       sleep_ms(20);

    }
    else if(valor == 2 && flag1 == 1)
    {
       gpio_put(EN_A, false);
       flag1 = 0;
       sleep_ms(20);
    }
    
    switch (valor)  // 'valor' é definido na função mqtt_dados_recebidos_cb
    {
       case 1: // SH - Sentido horário
       
          gpio_put(A1, true);  
          gpio_put(A2, false);
          valor = 0; // Reseta o valor para evitar repetição
          //printf("Sentido horario do motor\n");
       break;
    
       case 2: // AH - Sentido anti-horário
          gpio_put(A1, false); 
          gpio_put(A2, true);
          valor = 0; // Reseta o valor para evitar repetição
          // printf("Sentido anti-horario do motor\n");
       break;
    
       case 3: // + - Aumentar a intensidade
          
          if(duty_cycle < 1) {
           
            duty_cycle += 0.1;
          }  
          else if(duty_cycle >= 1)
          {
             printf("Atingiu o limite máximo da rotação\n");
          }
          valor = 0; // Reseta o valor para evitar repetição

       break;

       case 4: // -  Diminuir a intensidade
          if(duty_cycle >= 0.1) {

             duty_cycle -= 0.1;
          }
          else if(duty_cycle < 0.1)
          {
             duty_cycle = 0; // Garante que o duty cycle não fique negativo
             printf("Atingiu o limite mínimo da rotação\n");
          }
          valor = 0; // Reseta o valor para evitar repetição
       break;
    }
    set_duty_cycle_prop(duty_cycle);
    sleep_ms(10);    
}

int main()
{
    stdio_init_all();
    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
    // Enable wifi station
    cyw43_arch_enable_sta_mode();
    sleep_ms(5000);
    printf("Connecting to Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms("jag 2g", "180579ja", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n");
        // Read the ip address in a human readable way
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }
    /*
     * Configura o Endereço do Broker MQTT
     */
    ip_addr_t endereco_broker;
    ip4addr_aton("35.172.255.228", &endereco_broker);

    /*
     * Cria o ponteiro para representar o cliente MQTT
     */

    mqtt_client_t* mqtt_cliente = mqtt_client_new();
    
    mqtt_set_inpub_callback(mqtt_cliente, &mqtt_chegando_publicacao_cb, mqtt_dados_recebidos_cb, NULL);


    err_t houve_erro = mqtt_client_connect(mqtt_cliente, &endereco_broker, 1883, &mqtt_conectado_cb, NULL, &info_cliente);
    if (houve_erro != ERR_OK) {
        printf("Falha na requisição de conexão MQTT\n");
        return 0;
    }
    init_io_mode();
    init_pwm_mode();

    while (true) 
    {
       
       op_pwm_mode();
       cyw43_arch_poll();
       sleep_ms(10);
       
    }
}
