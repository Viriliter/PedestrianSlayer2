#ifndef UARTDRIVER_H
#define UARTDRIVER_H

#include <vector>

#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "baudrate.h"
#include "../configuration.h"
#include "container.h"

namespace HAL{
    struct uartConfig{
        uart_inst_t *id;
        BaudrateEnum baudrate;
        unsigned int tx_pin;
        unsigned int rx_pin;
    };
    
    class UARTDriver{
    private:
        uart_inst_t *uart_id;
        BaudrateEnum baudrate;
        unsigned int tx_pin;
        unsigned int rx_pin;

    public:
        UARTDriver(){};

        UARTDriver(uartConfig &config){
            uart_id = config.id;
            baudrate = config.baudrate;
            tx_pin = config.tx_pin;
            rx_pin = config.rx_pin;

            uart_init(uart_id, (uint)baudrate);

            // Set the TX and RX pins by using the function select on the GPIO
            // Set datasheet for more information on function select
            gpio_set_function(tx_pin, GPIO_FUNC_UART);
            gpio_set_function(rx_pin, GPIO_FUNC_UART);

        };

        void setID(uart_inst_t *uart_id_){
            uart_id = (uart_inst_t *) uart_id_;
        };

        void setBaudrate(BaudrateEnum baudrate_){
            baudrate = baudrate_;
            //uart_set_baudrate(MASTER_UART_ID, (uint)baudrate);
            uart_deinit(uart_id);
        };

        void read(char *buf, unsigned int bufSize){
            for(size_t i=0; i<bufSize; i++){
                buf[i] = uart_getc(uart_id);
            }
        };

        void read(UINT8 *buf, unsigned int bufSize){
            for(size_t i=0; i<bufSize; i++){
                buf[i] = (int) uart_getc(uart_id);
            }
        };

        void read(container::LinkedList<UINT8> &buf, unsigned int readSize){
            for(size_t i=0; i<readSize; i++){
                buf.push((UINT8) uart_getc(uart_id));
            }
        };

        void write(char *buf, unsigned int bufSize){
            for(size_t i=0; i<bufSize; i++){
                uart_putc_raw(uart_id, buf[i]);
            }
        };

        void write(UINT8 *buf, unsigned int bufSize){
            for(size_t i=0; i<bufSize; i++){
                uart_putc_raw(uart_id, (char) buf[i]);
            }
        };

        void write(container::LinkedList<UINT8> &buf){
            for(size_t i=0; i<buf.Count(); i++){
                uart_putc_raw(uart_id, (char) buf[i]);
            }
        };

        bool isAvailable(){
            // Check immediately (0 microseconds)
            return uart_is_readable_within_us(uart_id, 0);
        };
    };
}



#endif // UARTDRIVER_H