#ifndef SPIDRIVER_H
#define SPIDRIVER_H

#include <vector>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"

#include "../configuration.h"
#include "container.h"

namespace HAL{
    struct spiConfig{
        spi_inst_t *id;
        BaudrateEnum baudrate;
        unsigned int miso_pin;
        unsigned int cs_pin;
        unsigned int sck_pin;
        unsigned int mosi_pin;  
    };
    

    class SPIDriver{
    private:
        spi_inst_t *spi_id;
        BaudrateEnum baudrate;
        unsigned int miso_pin;
        unsigned int cs_pin;
        unsigned int sck_pin;
        unsigned int mosi_pin;      

        inline void cs_select() {
            asm volatile("nop \n nop \n nop");
            gpio_put(cs_pin, 0);  // Active low
            asm volatile("nop \n nop \n nop");
        }

        inline void cs_deselect() {
            asm volatile("nop \n nop \n nop");
            gpio_put(cs_pin, 1);
            asm volatile("nop \n nop \n nop");
        }

    public:
        SPIDriver(){};

        SPIDriver(spiConfig &config){
            spi_id = config.id;
            baudrate = config.baudrate;
            miso_pin = config.miso_pin;
            cs_pin = config.cs_pin;
            sck_pin = config.sck_pin;
            mosi_pin = config.mosi_pin;

            spi_init (spi_id, baudrate);
    
            gpio_set_function (miso_pin, GPIO_FUNC_SPI);
            gpio_set_function (sck_pin, GPIO_FUNC_SPI);
            gpio_set_function (mosi_pin, GPIO_FUNC_SPI);

            gpio_init(cs_pin);
            gpio_set_dir(cs_pin, GPIO_OUT);
            gpio_put(cs_pin, 1);
            gpio_set_function (cs_pin, GPIO_FUNC_SPI);

            // Make the CS pin available to picotool
            bi_decl(bi_1pin_with_name(cs_pin, "SPI CS"));

        };

        void setID(spi_inst_t *spi_id_){
            spi_id = (spi_inst_t *) spi_id_;
        };

        void setBaudrate(BaudrateEnum baudrate_){
            baudrate = baudrate_;
            spi_set_baudrate(spi_id, (uint)baudrate);
        };

        void connect(){
            spi_init(spi_id, baudrate);

            gpio_put(20, true);
        };

        void reconnect(){
            //Not implemented
        };

        void read(char *buf, unsigned int bufSize){
            cs_select();
            UINT8 *temp_buf = new UINT8[bufSize];

            spi_read_blocking (spi_id, 0, temp_buf, bufSize);
            
            for(size_t i=0; i<bufSize; i++){
                buf[i] = (char) temp_buf[i];
            }
            delete[] temp_buf;

            cs_deselect();
        };

        void read(UINT8 *buf, unsigned int bufSize){
            cs_select();

            spi_read_blocking (spi_id, 0, buf, bufSize);

            cs_deselect();
        };

        void read(container::LinkedList<UINT8> &buf, unsigned int readSize){
            cs_select();
            UINT8 *temp_buf = new UINT8[buf.Count()];

            spi_read_blocking (spi_id, 0, temp_buf, readSize);

            for(size_t i=0; i<readSize; i++){
                buf.push((UINT8) temp_buf[i]);
            }

            delete[] temp_buf;

            cs_deselect();
        };

        void write(char *buf, unsigned int bufSize){
            cs_select();
            UINT8 *temp_buf = new UINT8[bufSize];

            for(size_t i=0; i<bufSize; i++){
                temp_buf[i] = (UINT8) buf[i];
            }
            
            spi_write_blocking(spi_id, temp_buf, bufSize);

            delete[] temp_buf;

            cs_deselect();
        };

        void write(UINT8 *buf, unsigned int bufSize){
            cs_select();

            spi_write_blocking(spi_id, buf, bufSize);

            cs_deselect();
        };

        void write(container::LinkedList<UINT8> &buf){
            cs_select();
            UINT8 *temp_buf = new UINT8[buf.Count()];

            for(size_t i=0; i<buf.Count(); i++){
                temp_buf[i] = buf[i];
            }

            spi_write_blocking(spi_id, temp_buf, 1);
            delete[] temp_buf;

            cs_deselect();
        };

        bool isAvailable(){
            //Not implemented
            return true;
        };
    };
}

#endif // SPIDRIVER_H