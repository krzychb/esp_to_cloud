/*
 keenio.h - Routines to post data to Keen.io

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

#include "http.h"
#include "keenio.h"

static const char* TAG = "KeenIO";

/* Constants that aren't configurable in menuconfig
 */
#define WEB_SERVER "api.keen.io"

// The API key below is configurable in menuconfig
#define KEENIO_WRITE_API_KEY CONFIG_KEENIO_WRITE_API_KEY
#define KEENIO_REQUEST_URL CONFIG_KEENIO_REQUEST_URL
#define KEENIO_EVENT_COLLECTION CONFIG_KEENIO_EVENT_COLLECTION

static const char* get_request_start =
    "POST "KEENIO_REQUEST_URL" HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "Content-Type: application/json\n"
    "User-Agent: esp32 / esp-idf\n"
    "Authorization: "KEENIO_WRITE_API_KEY"\n";

static const char* get_request_end =
     "Connection: close\n"
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
    // const char * response_body = find_response_body(client->proc_buf);
    // printf("%s\n", response_body);

    free(client->proc_buf);
    client->proc_buf = NULL;
    client->proc_buf_size = 0;

    ESP_LOGD(TAG, "Free heap %u", xPortGetFreeHeapSize());
}

esp_err_t keenio_post_data(to_cloud *data_record, unsigned long record_count)
{

    int json_lenght = 0;
    char * json_start = "{\"esp-to-keenio-check\":[";
    char * json_end = "]}";
    json_lenght += strlen(json_start);
    json_lenght += strlen(json_end);
    // line below is to account for the last comma from string being removed
    json_lenght -= 1;  // '\0' - space for string termination character

    char * json_record_template =
            "{\"Up Time\":%lu,"
            "\"Heap Size\":%lu,"
            "\"Transmission Attempts\":%lu,"
            "\"Reply Timeouts\":%lu,"
            "\"Connection Failures\":%lu,"
            "\"Posting Time\":%lu,"
            "\"Wi-Fi Network Lost\":%lu,"
            "\"Post Failures\":%lu},";

    int json_record_length = 0;
    // total calculate size of json query string
    for (unsigned long i=0; i<record_count; i++) {
        int str_lenght = snprintf(NULL, 0, json_record_template,
                data_record[i].up_time,
                data_record[i].heap_size,
                data_record[i].transmission_attempts,
                data_record[i].reply_timeouts,
                data_record[i].connection_failures,
                data_record[i].posting_time,
                data_record[i].wifi_network_lost,
                data_record[i].post_failures);
        if (str_lenght > json_record_length) {
            json_record_length = str_lenght;
        }
        json_lenght += str_lenght;
    }

    // Content length update
    int n = snprintf(NULL, 0, "Content-Length: %d\n", json_lenght);
    char header_contenth_lenght[n+1];
    sprintf(header_contenth_lenght, "Content-Length: %d\n", json_lenght);

    // Request string size calculation
    int string_size = strlen(get_request_start);
    string_size += strlen(header_contenth_lenght);
    string_size += strlen(get_request_end);
    string_size += json_lenght;
    string_size += 1;  // '\0' - space for string termination character

    // Put the pieces of request string and JSON query together
    char * get_request = malloc(string_size);
    strcpy(get_request, get_request_start);
    strcat(get_request, header_contenth_lenght);
    strcat(get_request, get_request_end);
    strcat(get_request, json_start);

    char json_record[json_record_length];
    for (unsigned long i=0; i<record_count; i++) {
        sprintf(json_record, json_record_template,
                data_record[i].up_time,
                data_record[i].heap_size,
                data_record[i].transmission_attempts,
                data_record[i].reply_timeouts,
                data_record[i].connection_failures,
                data_record[i].posting_time,
                data_record[i].wifi_network_lost,
                data_record[i].post_failures);
        strcat(get_request, json_record);
    }
    // remove last comma / overwrite it with string termination character
    get_request[strlen(get_request)-1] = '\0';
    strcat(get_request, json_end);

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

void keenio_initialise()
{
    http_client_on_process_chunk(&http_client, process_chunk);
    http_client_on_disconnected(&http_client, disconnected);
}
