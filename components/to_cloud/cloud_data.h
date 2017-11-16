/*
 to_cloud.h - Definition of data transmitted to the cloud

 This file is part of the ESP32 Everest Run project
 https://github.com/krzychb/esp32-everest-run

 Copyright (c) 2016 Krzysztof Budzynski <krzychb@gazeta.pl>
 This work is licensed under the Apache License, Version 2.0, January 2004
 See the file LICENSE for details.
*/

#ifndef TO_CLOUD_H
#define TO_CLOUD_H

#include "esp_err.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define RED_BLINK_GPIO CONFIG_RED_BLINK_GPIO
#define GREEN_BLINK_GPIO CONFIG_GREEN_BLINK_GPIO
#define BLUE_BLINK_GPIO CONFIG_BLUE_BLINK_GPIO


/*
 * Structure of data transmitted to the cloud
 *
 * up_time               : up time in minutes
 * heap_size             : free heap size in bytes
 * transmission_attempts : number of transmission attempts to the cloud service
 * reply_timeouts        : number of reply timeouts by the cloud service
 * connection_failures   : number of failures to connect to the cloud
 * posting_time          : time in milliseconds that took to transmit the data and
 *                         obtain a reply from the service in previous post request
 * wifi_network_lost     : number of times Wi-Fi network was lost
 * post_failures         : number of failures posting data to the cloud
*/

typedef struct {
    unsigned long up_time;
    unsigned long heap_size;
    unsigned long transmission_attempts;
    unsigned long reply_timeouts;
    unsigned long connection_failures;
    unsigned long posting_time;
    unsigned long wifi_network_lost;
    unsigned long post_failures;
} to_cloud;

#ifdef __cplusplus
}
#endif

#endif  // TO_CLOUD_H
