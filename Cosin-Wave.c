
#include <math.h>
#include <assert.h>
#include "driver/gpio.h"
#include "driver/dac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/spi_master.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

static const char *TAG = "MQTT_TCP";

static esp_mqtt_client_handle_t client = NULL;

char topic[50];
char freq[50];
char offset[50];
int freq_;
int offset_;


void cosine(int freq, int offset){

    dac_cw_config_t cw_cfg = {
    .en_ch = DAC_CHANNEL_1,                      //Select which channel you want to enable 
    .scale = DAC_CW_SCALE_2,                     //The multiple of the amplitude of the cosine wave generator  
    .phase = DAC_CW_PHASE_180,                   //Set the phase of the cosine wave generator output.
    .freq = freq,                                //Set frequency of cosine wave generator output. Range:130(130Hz) ~ 55000(100KHz)
    .offset = offset,                            //Set the voltage value of the DC component of the cosine wave generator output. Range: -128 ~ 127
    };

    dac_output_enable(DAC_CHANNEL_1);
    dac_cw_generator_enable(); 
    dac_cw_generator_config(&cw_cfg); 

}

// MQTT functions

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(event->client, "freq", 0);
        esp_mqtt_client_subscribe(event->client, "offset", 0);
       //printf("Subscribed to two topics");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        sprintf(topic, "%.*s", event->topic_len, event->topic);
        printf("%s\n", (char*) topic);
                
        if(strcmp(topic, "freq")==0){
            sprintf(freq, "%.*s", event->data_len, event->data);
            freq_=atoi(freq);
            printf("offset:%i",freq_);
            cosine(freq_,offset_);
            //vTaskDelay(2000 / portTICK_PERIOD_MS);

        }

        if(strcmp(topic, "offset")==0){
            sprintf(offset, "%.*s", event->data_len, event->data);
            offset_=atoi(offset);
            cosine(freq_,offset_);
            printf("offset:%i",offset_);
            //TaskDelay(2000 / portTICK_PERIOD_MS);
        }
        
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://mqtt.eclipseprojects.io"
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation                   s1.1
    esp_event_loop_create_default();     // event loop                          s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                        s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //                                         s1.4
    // 2 - Wi-Fi Configuration Phase

esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "Hussam",
            .password = "12345678"}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
}


void app_main(void)
{
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    mqtt_app_start();
    printf("WIFI was initiated ...........\n\n");    
}




