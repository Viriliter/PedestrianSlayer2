/*
#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
*/

#include <time.h>
#include <vector>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "configuration.h"
#include "UARTDriver.h"
#include "SerialComm.h"
#include "container.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
uint8_t msgCounter = 0;
container::LinkedList<unsigned int> remaining_rx_buffer;

void loop();

clock_t clock_us()
{
    return (clock_t) time_us_64();
}

clock_t lastLightControl = clock_us();
clock_t lastSteerControl = clock_us();
clock_t lastThrustControl = clock_us();
clock_t lastCommControl = clock_us();
comm::SerialComm masterSerialComm{};

int main() {
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(18);
    gpio_set_dir(18, GPIO_OUT);

    gpio_init(20);
    gpio_set_dir(20, GPIO_OUT);

    masterSerialComm.connectPort(MASTER_UART_ID, HAL::BaudrateEnum::B115200);
    
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(MASTER_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MASTER_UART_RX_PIN, GPIO_FUNC_UART);

    loop();
}

void lightControl(long unsigned int period_ms){
    if ((clock_us() - lastLightControl) / 1000 < period_ms) return;
    lastLightControl = clock_us(); 

}

void steerControl(long unsigned int period_ms){
    if ((clock_us() - lastSteerControl) / 1000 < period_ms) return;
    lastSteerControl = clock_us(); 
    
}

void thrustControl(long unsigned int period_ms){
    if ((clock_us() - lastThrustControl) / 1000 < period_ms) return;
    lastThrustControl = clock_us(); 
}

void commControl(long unsigned int period_ms){
    if ((clock_us() - lastCommControl) / 1000 < period_ms) return;
    lastCommControl = clock_us();
    
    std::vector<message::SerialMessage*> rx_msgs;
    masterSerialComm.readIncomming(rx_msgs);

    for(auto &in_msg:rx_msgs){
        if (in_msg->msgID == 0x01){
            
        }

    }
    
    
    /*if (remaining_rx_buffer.Count()>0){
        if (remaining_rx_buffer[0] != 0xAA){
            //in_msg++;
        }
    }*/

    /*
    int msg[6]= {0xAA, 0x06, 0x81, 0b10000011, 0, 0};
    msgCounter %= 255;
    msg[4] = msgCounter;
    
    int checksum = 0;
    for (int i=0; i<5; i++){
        checksum += msg[i];
    }
    msg[5] = checksum%255;
    
    uart_putc_raw(MASTER_UART_ID, 'A');
    */

    message::RoverState tx_msg{};
    msgCounter %= 255; 
    
    tx_msg.addPayload(0b00000111, msgCounter);
    
    masterSerialComm.writeOutgoing(tx_msg);
    
    msgCounter++;
}

void loop() {
    while (1)
    {
        commControl(100);
        thrustControl(20);
        steerControl(20);
        lightControl(500);
    }
}
