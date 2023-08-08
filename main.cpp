#include <iostream>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"

#include "configuration.h"
#include "boards.h"
#include "./include/UARTDriver.h"

static uint thrust_slice_num;
static uint steering_slice_num;

static UINT8 s_thrustDutyCycle;
static UINT8 s_steeringAngle;

static UINT8 msgCounter = 0;

#define PWM_THRUST_WRAP (THRUST_PWM_PERIOD_US)*1000/(PWM_CLOCK_NS)


void on_motor_pwm(UINT8 thrustDutyCycle){
    double dutyCycle;
    if (thrustDutyCycle > 127){
        // Forward
        dutyCycle = (thrustDutyCycle - 127)/127.0;

        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_B, 0);
        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_A, (UINT16) PWM_THRUST_WRAP*dutyCycle);
    }
    else if (thrustDutyCycle < 127){
        // Backward
        dutyCycle = (127-thrustDutyCycle)/127.0;
        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_A, 0);
        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_B, (UINT16) PWM_THRUST_WRAP*dutyCycle);
    }
    else{
        // Stop
        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_A, 0);
        pwm_set_chan_level(thrust_slice_num, PWM_CHAN_B, 0);
    }
}

void on_steering_pwm(UINT8 steeringDutyCycle){
    double dutyCycle;
    if (steeringDutyCycle > 127){
        // Right
        dutyCycle = (steeringDutyCycle - 127)/127.0;

        pwm_set_chan_level(steering_slice_num, PWM_CHAN_B, 0);
        pwm_set_chan_level(steering_slice_num, PWM_CHAN_A, (UINT16) PWM_THRUST_WRAP*dutyCycle);
    }
    else if (steeringDutyCycle < 127){
        // Left
        dutyCycle = (127-steeringDutyCycle)/127.0;
        pwm_set_chan_level(steering_slice_num, PWM_CHAN_A, 0);
        pwm_set_chan_level(steering_slice_num, PWM_CHAN_B, (UINT16) PWM_THRUST_WRAP*dutyCycle);
    }
    else{
        // Neutral
        pwm_set_chan_level(steering_slice_num, PWM_CHAN_A, 0);
        pwm_set_chan_level(steering_slice_num, PWM_CHAN_B, 0);
    }
}

int main(){
    stdio_init_all();

    // IO

    //gpio_init(ONBOARD_LED);
    //gpio_set_dir(ONBOARD_LED, GPIO_OUT);
    gpio_init(PIN_RIGHT_LIGHT);
    gpio_set_dir(PIN_RIGHT_LIGHT, GPIO_OUT);

    gpio_init(PIN_LEFT_LIGHT);
    gpio_set_dir(PIN_LEFT_LIGHT, GPIO_OUT);
    
    // PWM
    gpio_set_function(PWM_THRUST_FORWARD, GPIO_FUNC_PWM);
    gpio_set_function(PWM_THRUST_BACKWARD, GPIO_FUNC_PWM);
    thrust_slice_num = pwm_gpio_to_slice_num(PWM_THRUST_FORWARD);

    gpio_set_function(PWM_STEERING_LEFT, GPIO_FUNC_PWM);
    gpio_set_function(PWM_STEERING_RIGHT, GPIO_FUNC_PWM);
    steering_slice_num = pwm_gpio_to_slice_num(PWM_STEERING_LEFT);

    pwm_set_enabled(thrust_slice_num, true);
    pwm_set_enabled(steering_slice_num, true);

    pwm_set_wrap(thrust_slice_num, PWM_THRUST_WRAP);  // 4us/8ns = 500
    pwm_set_wrap(steering_slice_num, PWM_THRUST_WRAP);

    // Serial
    HAL::UARTDriver driver(MASTER_UART_ID, HAL::BaudrateEnum::B115200);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(MASTER_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MASTER_UART_RX_PIN, GPIO_FUNC_UART);
    on_motor_pwm(127);
    on_steering_pwm(127);

    while(true){
        if(driver.isAvailable()){
            char *rx_buf = new char[7];
            driver.read(rx_buf, 7);
            UINT8 roverControl = rx_buf[3];
            UINT8 steeringAngle = rx_buf[4];
            UINT8 thrustDutyCycle = rx_buf[5];

            bool isArmed = bool(roverControl & 0b00000001);  // Arm / Disarm
            bool isLeftLightOn = bool(roverControl & 0b00000010);  // Left light control
            bool isRightLightOn = bool(roverControl & 0b00000100);  // Right light control
            bool isEmergencyStopEnabled = bool(roverControl & 0b01000000);  // Right light control

            if (isArmed && !isEmergencyStopEnabled){
                on_motor_pwm(thrustDutyCycle);
                on_steering_pwm(steeringAngle);
            }

            msgCounter = (msgCounter+1)%256;
            char *tx_buf = new char[6];
            tx_buf[0] = 0xAA;
            tx_buf[1] = 0x06;
            tx_buf[2] = 0x81;
            tx_buf[3] = roverControl;
            tx_buf[4] = msgCounter;
            tx_buf[5] = (tx_buf[0]+tx_buf[1]+tx_buf[2]+tx_buf[3]+tx_buf[4])%255;
            driver.write(tx_buf, 6);

            delete[] tx_buf;
            delete[] rx_buf;
            
        }
    }

    return 0;
}
