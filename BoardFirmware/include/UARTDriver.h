#ifndef UARTDRIVER_H
#define UARTDRIVER_H

#include <vector>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "../configuration.h"
#include "container.h"

namespace HAL{
    enum BaudrateEnum{
        B300 = 300,
        B1200 = 1200,
        B2400 = 2400,
        B4800 = 4800,
        B9600 = 9600,
        B19200 = 19200,
        B38400 = 38400,
        B57600 = 57600,
        B74880 = 74880,
        B115200 = 115200,
        B230400 = 230400,
        B250000 = 250000
    };
    
    class UARTDriver{
    private:
        uart_inst_t *uart_id;
        BaudrateEnum baudrate;
        unsigned int tx_pin;
        unsigned int rx_pin;
        
    public:
        UARTDriver(){};

        UARTDriver(uart_inst_t *uart_id_, BaudrateEnum baudrate_):
        uart_id((uart_inst_t *) uart_id_), baudrate(baudrate_){
            uart_init(uart_id, (uint)baudrate);
        };

        void setUartID(uart_inst_t *uart_id_){
            uart_id = (uart_inst_t *) uart_id_;
        };

        void setBaudrate(BaudrateEnum baudrate_){
            baudrate = baudrate_;
            //uart_set_baudrate(MASTER_UART_ID, (uint)baudrate);
        };

        void connect(){
            uart_init(uart_id, 115200);

            gpio_put(20, true);
        };

        void reconnect(){
            uart_deinit(uart_id);
        };

        void read(char *buf, unsigned int bufSize){
            for(size_t i=0; i<bufSize; i++){
                buf[i] = uart_getc(uart_id);
            }
        };

        void read(int *buf, unsigned int bufSize){
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

        void write(int *buf, unsigned int bufSize){
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