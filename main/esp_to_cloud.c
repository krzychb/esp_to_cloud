/*
 esp_to_thingspeak.c - Post sample data to ThingSpeak

 This file is part of the ESP32 Everest Run project
 https://github.com/krzychb/esp32-everest-run

 Copyright (c) 2016 Krzysztof Budzynski <krzychb@gazeta.pl>
 This work is licensed under the Apache License, Version 2.0, January 2004
 See the file LICENSE for details.
*/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "wifi.h"
#include "cloud_data.h"
#include "thingspeak.h"
#include "keenio.h"

static const char* TAG_CLOUD = "To Cloud";
static const char* TAG_THINGSPEAK = "To ThingSpeak";
static const char* TAG_KEENIO = "To KeenIO";
#define SEND_TO_CLOUD_PERIOD 15000


void initialise_blink_leds()
{
    gpio_pad_select_gpio(RED_BLINK_GPIO);
    gpio_set_direction(RED_BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(GREEN_BLINK_GPIO);
    gpio_set_direction(GREEN_BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(BLUE_BLINK_GPIO);
    gpio_set_direction(BLUE_BLINK_GPIO, GPIO_MODE_OUTPUT);
}


static void send_to_thingspeak(void)
{
    static to_cloud data_record = {0};

    data_record.up_time = esp_log_timestamp()/1000l/60l;
    data_record.heap_size = xPortGetFreeHeapSize();
    data_record.transmission_attempts++;
    ESP_LOGI(TAG_THINGSPEAK, "Transmission attempts: %lu", data_record.transmission_attempts);

    gpio_set_level(RED_BLINK_GPIO, 0);
    if (network_is_alive() == true) {
        uint32_t start_transmission = esp_log_timestamp();
        esp_err_t err = thinkgspeak_post_data(&data_record);
        data_record.posting_time = esp_log_timestamp() - start_transmission;
        if (err != ESP_OK) {
            gpio_set_level(RED_BLINK_GPIO, 1);
            data_record.post_failures++;
        }
    } else {
        gpio_set_level(RED_BLINK_GPIO, 1);
        data_record.connection_failures++;
        ESP_LOGW(TAG_THINGSPEAK, "Wi-Fi network lost");
    }
}

static void send_to_keenio(void)
{
    static to_cloud data_record = {0};

    data_record.up_time = esp_log_timestamp()/1000l/60l;
    data_record.heap_size = xPortGetFreeHeapSize();
    data_record.transmission_attempts++;
    ESP_LOGI(TAG_KEENIO, "Transmission attempts: %lu", data_record.transmission_attempts);

    gpio_set_level(RED_BLINK_GPIO, 0);
    if (network_is_alive() == true) {
        uint32_t start_transmission = esp_log_timestamp();
        esp_err_t err = keenio_post_data(&data_record, 1);
        data_record.posting_time = esp_log_timestamp() - start_transmission;
        if (err != ESP_OK) {
            gpio_set_level(RED_BLINK_GPIO, 1);
            data_record.post_failures++;
        }
    } else {
        gpio_set_level(RED_BLINK_GPIO, 1);
        data_record.connection_failures++;
        ESP_LOGW(TAG_KEENIO, "Wi-Fi network lost");
    }
}


static void send_to_cloud_task(void *pvParameter)
{
    while (1) {
        send_to_thingspeak();
        send_to_keenio();
        vTaskDelay(SEND_TO_CLOUD_PERIOD / portTICK_PERIOD_MS);
    }
}


void app_main()
{
    ESP_LOGI(TAG_CLOUD, "Starting...");
    initialise_blink_leds();
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();

    while (network_is_alive() == false) {
        ESP_LOGI(TAG_CLOUD, "Wi-Fi network connection missing");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_CLOUD, "Now connected to Wi-Fi");

    thinkgspeak_initialise();
    ESP_LOGI(TAG_CLOUD, "Posting to ThingSpeak initialized");

    keenio_initialise();
    ESP_LOGI(TAG_CLOUD, "Posting to KeenIO initialized");

    xTaskCreate(&send_to_cloud_task, "send_to_cloud_task", 4 * 1024, NULL, 5, NULL);
}
