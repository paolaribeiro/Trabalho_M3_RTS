#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <driver/gpio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "sdkconfig.h"
#include "freertos/semphr.h"
#include "driver/touch_pad.h"
#include "nvs_flash.h"  
#include "nvs.h"

#define TOUCH_PAD_THRESHOLD 600
#define TOUCH_PAD_OLEO 9      //Interrupção do duto de oleo  D32
#define TOUCH_PAD_GAS 8       //Interrupção do duto de gas  D33
#define TOUCH_PAD_POCO 7      //Interrupção do poço  D27
#define ESTAVEL 1             //Estado do sensor
#define INSTAVEL 0            //Estado do sensor

static bool s_pad_activated[TOUCH_PAD_MAX];

SemaphoreHandle_t sensor_duto_mutex;
SemaphoreHandle_t log_buffer_mutex;

int sensor_poco = ESTAVEL; //Inicialização do sensor como estavel
int sensor_duto = ESTAVEL; //Inicialização do sensor como estavel

unsigned log_buffer_size = 0;
char **log_buffer = NULL;

void PressaoPoco_Instavel(){ 
    uint64_t begin, end;
    begin = esp_timer_get_time(); //Pega o horário de inicio
    sensor_poco = INSTAVEL;
    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);

    if (log_buffer == NULL){
        log_buffer = malloc(6 * sizeof(char*));
        log_buffer_size = 6;

        for (int i = 0; i < 6; i++){
            log_buffer[i] = malloc(40 * sizeof(char));
            
            if (i == 0){
                log_buffer[i] = "Sensor do poço instável";         //Mensagem a ser exibida
            } else if (i == 1){
                log_buffer[i] = "Aplicação de contra medida.";     //Mensagem a ser exibida
            } else if (i == 2){
                log_buffer[i] = "Aplicação de contra medida..";    //Mensagem a ser exibida
            } else if (i == 3){
                log_buffer[i] = "Aplicação de contra medida...";   //Mensagem a ser exibida
            } else if (i == 4){
                log_buffer[i] = "Contra medida concluída";         //Mensagem a ser exibida
            }
        }
    } else {
        char **log_buffer_new = NULL;
        unsigned log_buffer_new_size = log_buffer_size + 6;

        log_buffer_new = malloc(log_buffer_new_size * sizeof(char*));

        for (unsigned i = 0; i < log_buffer_size; i++){
            log_buffer_new[i] = log_buffer[i];
        }

        for (unsigned i = log_buffer_size; i < log_buffer_new_size; i++){
            log_buffer_new[i] = malloc(40 * sizeof(char));

            if (i == log_buffer_size){
                log_buffer[i] = "Sensor do poço instável";         //Mensagem a ser exibida
            } else if (i == log_buffer_size + 1){
                log_buffer[i] = "Aplicação de contra medida.";     //Mensagem a ser exibida
            } else if (i == log_buffer_size + 2){
                log_buffer[i] = "Aplicação de contra medida..";    //Mensagem a ser exibida
            } else if (i == log_buffer_size + 3){
                log_buffer[i] = "Aplicação de contra medida...";   //Mensagem a ser exibida
            } else if (i == log_buffer_size + 4){
                log_buffer[i] = "Contra medida concluída";         //Mensagem a ser exibida
            }
        }

        for (int i = 0; i < log_buffer_size; i++){
            free(log_buffer[i]);
        }

        free(log_buffer);
        log_buffer = log_buffer_new;
        log_buffer_size = log_buffer_new_size;
    }

    xSemaphoreGive(log_buffer_mutex);
    sensor_poco = ESTAVEL;
    end = esp_timer_get_time(); //Pega o horário do final

    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);
    sprintf(log_buffer[log_buffer_size - 1], "Tempo: %llu\n", (end - begin)); //Calculo do tempo utilizado
    xSemaphoreGive(log_buffer_mutex);
}

void PressaoGas_Instavel(){
    uint64_t begin, end;
    begin = esp_timer_get_time();  //Pega o horário de inicio
    
    xSemaphoreTake(sensor_duto_mutex, portMAX_DELAY);
    sensor_duto = INSTAVEL;
    xSemaphoreGive(sensor_duto_mutex);
    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);

    if (log_buffer == NULL){
        log_buffer = malloc(6 * sizeof(char*));
        log_buffer_size = 6;

        for (int i = 0; i < 6; i++){
            log_buffer[i] = malloc(40 * sizeof(char));
            
            if (i == 0){
                log_buffer[i] = "Sensor do duto de gás instável";   //Mensagem a ser exibida
            } else if (i == 1){
                log_buffer[i] = "Aplicação de contra medida.";      //Mensagem a ser exibida
            } else if (i == 2){
                log_buffer[i] = "Aplicação de contra medida..";     //Mensagem a ser exibida
            } else if (i == 3){
                log_buffer[i] = "Aplicação de contra medida...";    //Mensagem a ser exibida
            } else if (i == 4){
                log_buffer[i] = "Contra medida concluída";          //Mensagem a ser exibida
            }
        }
    } else {
        char **log_buffer_new = NULL;
        unsigned log_buffer_new_size = log_buffer_size + 6;

        log_buffer_new = malloc(log_buffer_new_size * sizeof(char*));

        for (unsigned i = 0; i < log_buffer_size; i++){
            log_buffer_new[i] = log_buffer[i];
        }

        for (unsigned i = log_buffer_size; i < log_buffer_new_size; i++){
            log_buffer_new[i] = malloc(40 * sizeof(char));

            if (i == log_buffer_size){
                log_buffer[i] = "Sensor do duto de gás instável";  //Mensagem a ser exibida
            } else if (i == log_buffer_size + 1){
                log_buffer[i] = "Aplicação de contra medida.";     //Mensagem a ser exibida
            } else if (i == log_buffer_size + 2){
                log_buffer[i] = "Aplicação de contra medida..";    //Mensagem a ser exibida
            } else if (i == log_buffer_size + 3){
                log_buffer[i] = "Aplicação de contra medida...";   //Mensagem a ser exibida
            } else if (i == log_buffer_size + 4){
                log_buffer[i] = "Contra medida concluída";         //Mensagem a ser exibida
            }
        }

        for (int i = 0; i < log_buffer_size; i++){
            free(log_buffer[i]);
        }

        free(log_buffer);
        log_buffer = log_buffer_new;
        log_buffer_size = log_buffer_new_size;
    }

    xSemaphoreGive(log_buffer_mutex);
    xSemaphoreTake(sensor_duto_mutex, portMAX_DELAY);
    sensor_duto = ESTAVEL;
    xSemaphoreGive(sensor_duto_mutex);

    end = esp_timer_get_time();   //Pega o horário do final
    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);
    sprintf(log_buffer[log_buffer_size - 1], "Tempo: %llu\n", (end - begin));  //Calculo do tempo utilizado
    xSemaphoreGive(log_buffer_mutex);
}

void PressaoOleo_Instavel(){
    uint64_t begin, end;
    begin = esp_timer_get_time();    //Pega o horário de inicio
    
    xSemaphoreTake(sensor_duto_mutex, portMAX_DELAY);
    sensor_duto = INSTAVEL;
    xSemaphoreGive(sensor_duto_mutex);
    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);

    if (log_buffer == NULL){
        log_buffer = malloc(6 * sizeof(char*));
        log_buffer_size = 6;

        for (int i = 0; i < 6; i++){
            log_buffer[i] = malloc(40 * sizeof(char));
            
            if (i == 0){
                log_buffer[i] = "Sensor do duto de óleo instável";   //Mensagem a ser exibida
            } else if (i == 1){
                log_buffer[i] = "Aplicação de contra medida.";       //Mensagem a ser exibida
            } else if (i == 2){
                log_buffer[i] = "Aplicação de contra medida..";      //Mensagem a ser exibida
            } else if (i == 3){
                log_buffer[i] = "Aplicação de contra medida...";     //Mensagem a ser exibida
            } else if (i == 4){
                log_buffer[i] = "Contra medida concluída";           //Mensagem a ser exibida
            }
        }
    } else {
        char **log_buffer_new = NULL;
        unsigned log_buffer_new_size = log_buffer_size + 6;

        log_buffer_new = malloc(log_buffer_new_size * sizeof(char*));

        for (unsigned i = 0; i < log_buffer_size; i++){
            log_buffer_new[i] = log_buffer[i];
        }

        for (unsigned i = log_buffer_size; i < log_buffer_new_size; i++){
            log_buffer_new[i] = malloc(40 * sizeof(char));

            if (i == log_buffer_size){
                log_buffer[i] = "Sensor do duto de óleo instável";     //Mensagem a ser exibida
            } else if (i == log_buffer_size + 1){
                log_buffer[i] = "Aplicação de contra medida.";         //Mensagem a ser exibida
            } else if (i == log_buffer_size + 2){
                log_buffer[i] = "Aplicação de contra medida..";        //Mensagem a ser exibida
            } else if (i == log_buffer_size + 3){
                log_buffer[i] = "Aplicação de contra medida...";       //Mensagem a ser exibida
            } else if (i == log_buffer_size + 4){
                log_buffer[i] = "Contra medida concluída";             //Mensagem a ser exibida
            }
        }

        for (int i = 0; i < log_buffer_size; i++){
            free(log_buffer[i]);
        }

        free(log_buffer);
        log_buffer = log_buffer_new;
        log_buffer_size = log_buffer_new_size;
    }

    xSemaphoreGive(log_buffer_mutex);
    xSemaphoreTake(sensor_duto_mutex, portMAX_DELAY);
    sensor_duto = ESTAVEL;
    xSemaphoreGive(sensor_duto_mutex);

    end = esp_timer_get_time();  //Pega o horário de inicio
    xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);
    sprintf(log_buffer[log_buffer_size - 1], "Tempo: %llu\n", (end - begin));  //Calculo do tempo utilizado
    xSemaphoreGive(log_buffer_mutex);
}

void Display(void *pvParameter){
    while (1){
        xSemaphoreTake(log_buffer_mutex, portMAX_DELAY);

        for (unsigned i = 0; i < log_buffer_size; i++){
            printf("%s\n", log_buffer[i]);
        }

        free(log_buffer);
        log_buffer = NULL;
        log_buffer_size = 0;
        xSemaphoreGive(log_buffer_mutex);

        if (sensor_poco == ESTAVEL){
            printf("Sensor poço estável\n");   //Mensagem a ser exibida
        }

        if (sensor_duto == ESTAVEL){
            printf("Sensores duto óleo e gás estáveis\n\n");   //Mensagem a ser exibida
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void TouchPad_Sensores(){
    touch_pad_config(TOUCH_PAD_POCO, TOUCH_PAD_THRESHOLD);
    touch_pad_config(TOUCH_PAD_GAS, TOUCH_PAD_THRESHOLD);
    touch_pad_config(TOUCH_PAD_OLEO, TOUCH_PAD_THRESHOLD);
}

void TouchPad_intr(void *arg){
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status();

    for (int i = 0; i < TOUCH_PAD_MAX; i++){
        if ((pad_intr >> i) & 0x01){
            s_pad_activated[i] = true;
        }
    }
}

void Identificao_TouchPad(void *pvParameter){

    while (1){
        touch_pad_intr_enable();
        for (int i = 0; i < TOUCH_PAD_MAX; i++){
            if (s_pad_activated[i] == true){ //Caso tenha algum touch_pad ativo, verifica e causa a interrupção
                if (i == TOUCH_PAD_POCO){
                    PressaoPoco_Instavel();    
                } else if (i == TOUCH_PAD_GAS){
                    PressaoGas_Instavel();
                } else if (i == TOUCH_PAD_OLEO){
                    PressaoOleo_Instavel();
                }
                s_pad_activated[i] = false; //Desativando o touch_pad, "limpando informação"
            }
        }
        vTaskDelay(500/ portTICK_PERIOD_MS);
    }
}

void app_main(){
    nvs_flash_init();
    touch_pad_init();
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    TouchPad_Sensores();
    touch_pad_isr_register(TouchPad_intr, NULL);

    sensor_duto_mutex = xSemaphoreCreateMutex();
    log_buffer_mutex = xSemaphoreCreateMutex();

    xTaskCreate(&Identificao_TouchPad, "Identificao_TouchPad", 2048, NULL, 5, NULL); //Criação das tarefas
    xTaskCreate(&Display, "Display", 2048, NULL, 2, NULL);           //Criação das tarefas
}