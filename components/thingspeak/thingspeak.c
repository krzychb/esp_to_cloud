/*
 thingspeak.c - Routines to post data to ThingSpeak.com

 This file is part of the ESP32 Everest Run project
 https://github.com/krzychb/esp32-everest-run

 Copyright (c) 2016 Krzysztof Budzynski <krzychb@gazeta.pl>
 This work is licensed under the Apache License, Version 2.0, January 2004
 See the file LICENSE for details.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#include "thingspeak.h"
#include "http.h"

static const char* TAG = "ThingSpeak";

/* Constants that aren't configurable in menuconfig
 */
#define WEB_SERVER "api.thingspeak.com"

// The API key below is configurable in menuconfig
#define THINGSPEAK_WRITE_API_KEY CONFIG_THINGSPEAK_WRITE_API_KEY

static const char* get_request_start =
    "GET /update?key="
    THINGSPEAK_WRITE_API_KEY;

static const char* get_request_end =
    " HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "Connection: close\n"
    "User-Agent: esp32 / esp-idf\n"
    "\n";

static http_client_data http_client = {0};

/* Collect chunks of data received from server
   into complete message and save it in proc_buf
 */
static void process_chunk(uint32_t *args)
{
    http_client_data* client = (http_client_data*)args;

    int proc_buf_new_size = client->proc_buf_size + client->recv_buf_size;
    char *copy_from;

    if (client->proc_buf == NULL){
        client->proc_buf = malloc(proc_buf_new_size);
        copy_from = client->proc_buf;
    } else {
        proc_buf_new_size -= 1; // chunks of data are '\0' terminated
        client->proc_buf = realloc(client->proc_buf, proc_buf_new_size);
        copy_from = client->proc_buf + proc_buf_new_size - client->recv_buf_size;
    }
    if (client->proc_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
    }
    client->proc_buf_size = proc_buf_new_size;
    memcpy(copy_from, client->recv_buf, client->recv_buf_size);
}

static void disconnected(uint32_t *args)
{
    http_client_data* client = (http_client_data*)args;

    // ToDo: REMOVE
    // print server's response
    // for diagnostic purposes
    //
    // printf("%s\n", client->proc_buf);

    free(client->proc_buf);
    client->proc_buf = NULL;
    client->proc_buf_size = 0;

    ESP_LOGD(TAG, "Free heap %u", xPortGetFreeHeapSize());
}

esp_err_t thinkgspeak_post_data(to_cloud *data_record)
{
    int n;

    // 1
    n = snprintf(NULL, 0, "%lu", data_record->up_time);
    char field1[n+1];
    sprintf(field1, "%lu", data_record->up_time);
    // 2
    n = snprintf(NULL, 0, "%lu", data_record->heap_size);
    char field2[n+1];
    sprintf(field2, "%lu", data_record->heap_size);
    // 3
    n = snprintf(NULL, 0, "%lu", data_record->transmission_attempts);
    char field3[n+1];
    sprintf(field3, "%lu", data_record->transmission_attempts);
    // 4
    n = snprintf(NULL, 0, "%lu", data_record->reply_timeouts);
    char field4[n+1];
    sprintf(field4, "%lu", data_record->reply_timeouts);
    // 5
    n = snprintf(NULL, 0, "%lu", data_record->connection_failures);
    char field5[n+1];
    sprintf(field5, "%lu", data_record->connection_failures);
    // 6
    n = snprintf(NULL, 0, "%lu", data_record->posting_time);
    char field6[n+1];
    sprintf(field6, "%lu", data_record->posting_time);
    // 7
    n = snprintf(NULL, 0, "%lu", data_record->wifi_network_lost);
    char field7[n+1];
    sprintf(field7, "%lu", data_record->wifi_network_lost);
    n = snprintf(NULL, 0, "%lu", data_record->post_failures);
    char field8[n+1];
    sprintf(field8, "%lu", data_record->post_failures);
    // request string size calculation
    int string_size = strlen(get_request_start);
    string_size += strlen("&fieldN=") * 8;  // number of fields
    string_size += strlen(field1);
    string_size += strlen(field2);
    string_size += strlen(field3);
    string_size += strlen(field4);
    string_size += strlen(field5);
    string_size += strlen(field6);
    string_size += strlen(field7);
    string_size += strlen(field8);
    string_size += strlen(get_request_end);
    string_size += 1;  // '\0' - space for string termination character

    // request string assembly / concatenation
    char * get_request = malloc(string_size);
    strcpy(get_request, get_request_start);
    strcat(get_request, "&field1=");
    strcat(get_request, field1);
    strcat(get_request, "&field2=");
    strcat(get_request, field2);
    strcat(get_request, "&field3=");
    strcat(get_request, field3);
    strcat(get_request, "&field4=");
    strcat(get_request, field4);
    strcat(get_request, "&field5=");
    strcat(get_request, field5);
    strcat(get_request, "&field6=");
    strcat(get_request, field6);
    strcat(get_request, "&field7=");
    strcat(get_request, field7);
    strcat(get_request, "&field8=");
    strcat(get_request, field8);
    strcat(get_request, get_request_end);

    // ToDo: REMOVE
    // print get request
    // for diagnostic purposes
    //
    // printf("%d, %s\n", string_size, get_request);

    gpio_set_level(BLUE_BLINK_GPIO, 1);
    esp_err_t err = http_client_request(&http_client, WEB_SERVER, get_request);
    gpio_set_level(BLUE_BLINK_GPIO, 0);

    free(get_request);
    return err;
}

void thinkgspeak_initialise()
{
    http_client_on_process_chunk(&http_client, process_chunk);
    http_client_on_disconnected(&http_client, disconnected);
}
