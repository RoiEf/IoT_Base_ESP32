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

typedef String (*funcPtr)(void);

enum WIFI_STATUS {
    DISCONNECTED,
    CONNECTED
};

typedef union {
    uint32_t decimal;
    uint8_t octecs[4];
} IP_CONVERT;

class ip_convert {
   public:
    IP_CONVERT ip;
    void reverse() {
        uint8_t tmp = ip.octecs[0];
        ip.octecs[0] = ip.octecs[3];
        ip.octecs[3] = tmp;
        tmp = ip.octecs[1];
        ip.octecs[1] = ip.octecs[2];
        ip.octecs[2] = tmp;
    }
};