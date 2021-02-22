#pragma once

#include <Arduino.h>

enum MODE {
    RESET,
    AP,
    DEVICE
};
typedef enum {
    PASS_BAD,
    PASS_OK,
    PASS_NONE
} PASSWORD;
typedef enum {
    DES_ROOT,
    DES_FEAT,
    DES_NET,
    DES_AP,
    DES_STA
} DESTINATION;

typedef union {
    uint32_t d_ip;
    uint8_t octecs[4];
} IP_CONVERT;

typedef String (*funcPtr)(void);

enum WIFI_STATUS {
    DISCONNECTED,
    CONNECTED
};
