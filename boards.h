#ifndef BOARDS_H
#define BOARDS_H

#include "configuration.h"
#include "include/baudrate.h"

#if BOARD_TYPE == PICO

#define ONBOARD_LED 25
#define PIN_LEFT_LIGHT 19
#define PIN_RIGHT_LIGHT 18
#define PWM_STEERING_LEFT 15  // PWM7 B
#define PWM_STEERING_RIGHT 14  // PWM7 A
#define PWM_THRUST_FORWARD 16  // PWM0 A 18
#define PWM_THRUST_BACKWARD 17  // PWM0 B 19

#define PWM_CLOCK_NS 8

#ifndef ENABLE_MASTER_SPI_COMM // Enable UART for master communication
    #define MASTER_UART_ID uart1
    #define MASTER_UART_BAUD_RATE BaudrateEnum::B115200

    #define MASTER_UART_TX_PIN 4
    #define MASTER_UART_RX_PIN 5
#else // Enable SPI for master communication
    #define MASTER_SPI_ID spi0
    #define MASTER_SPI_BAUD_RATE BaudrateEnum::MHz1  // 1MHz
    #define MASTER_SPI_MISO_PIN 3  // SPI0 TX
    #define MASTER_SPI_CS_PIN 5  // SPI0 Csn
    #define MASTER_SPI_SCK_PIN 2  // SPI0 SCK
    #define MASTER_SPI_MOSI_PIN 4  // SPI0 TX
#endif

#endif // PICO


#endif // CONFIGURATION_H