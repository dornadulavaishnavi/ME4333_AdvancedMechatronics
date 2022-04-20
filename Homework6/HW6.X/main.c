#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include "spi.h"
#include "UART.h"
#include "i2c_master_noint.h"

void PIC_INIT();
unsigned short Convertto16bit(char channel, unsigned char v);

void main() {
    //keep pic init, A4, B4, UART
    PIC_INIT();
    UART1_INIT();
    initSPI();
    i2c_master_setup();

    while (1) {
        //write to init
        //write to turn on gp7
        //read from gpio
        //check if button is pushed
    }
}

void PIC_INIT() {
    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    __builtin_enable_interrupts();

    //initialize pins and write to output
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;
}

unsigned short Convertto16bit(char channel, unsigned char voltage) {
    unsigned short s = 0;

    s = s | (channel << 15);
    s = s | (0b111 << 12);
    s = s | (voltage << 4);

    return s;
}

void Write_Message_I2C(unsigned char address, unsigned char reg, unsigned char to_send) {
    //send start bit
    i2c_master_start();
    //write the address with a write or read bit 
    i2c_master_send(ADDRESS << 1 |);
    //write register to change
    i2c_master_send(reg);
    //write value to change to
    i2c_master_send(to_send);
    //send stop bit to end communication
    i2c_master_stop();
}

unsigned char Read_Message_I2C(unsigned char address, unsigned char reg) {
    //send start bit
    i2c_master_start();
    //write the address with a write or read bit 
    i2c_master_send(ADDRESS << 1 |);
    //write register to change
    i2c_master_send(reg);
    //reset i2c
    i2c_master_restart();
    i2c_master_send(address<<1|0b1); //to read
    unsigned char ret = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    
    return ret;        
}

//questions for oh:
//    - what should init do?
//    - what is final goal to show?
//    - only hooking up scl, sda, reset to mclr, vss, and vdd. other than button and led, no ther sp0 pins irhgt?
//    - so adding another button but which led do we use
//    - how do we set the internal pull up resistor
        