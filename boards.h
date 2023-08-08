#ifndef BOARDS_H
#define BOARDS_H

#include "configuration.h"

#if BOARD_TYPE == PICO

#define ONBOARD_LED 25
#define PIN_LEFT_LIGHT 19
#define PIN_RIGHT_LIGHT 18
#define PWM_STEERING_LEFT 15  // PWM7 B
#define PWM_STEERING_RIGHT 14  // PWM7 A
#define PWM_THRUST_FORWARD 16  // PWM0 A 18
#define PWM_THRUST_BACKWARD 17  // PWM0 B 19

#define PWM_CLOCK_NS 8

#endif // PICO


#endif // CONFIGURATION_H