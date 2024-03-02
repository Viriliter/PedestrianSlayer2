#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// BOARD CONFIGS
#define BOARD_TYPE PICO

// COMMUNICATION CONFIGS
// Enables SPI interface for master communication.
// Comment following line to enable UART interface.
//#define ENABLE_MASTER_SPI_COMM

// CONTROL CONFIGS
#define TIME_STEP 10

#define THRUST_P 1
#define THRUST_I 0
#define THRUST_D 0
#define THRUST_MAX 100
#define THRUST_MIN -100

#define STEERING_P 1
#define STEERING_I 0
#define STEERING_D 0
#define STEERING_MAX 100
#define STEERING_MIN -100

#define THRUST_PWM_PERIOD_US 4

#endif // CONFIGURATION_H