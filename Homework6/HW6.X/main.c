#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include "spi.h"
#include "UART.h"
#include "i2c_master_noint.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF  // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use fast frc oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // primary osc disabled
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1 // use largest wdt value
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz fast rc internal oscillator
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 00000000 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

void PIC_INIT();
unsigned short Convertto16bit(char channel, unsigned char v);
void Write_Message_I2C(unsigned char address, unsigned char reg, unsigned char to_send);
unsigned char Read_Message_I2C(unsigned char address, unsigned char reg);
void blink_LED();

//global vars
int button_val = 0;

void main() {
    //keep pic init, A4, B4, UART
    PIC_INIT();
    UART1_INIT();
    i2c_master_setup();

    //init mcp23008 with gp7 as output and gp0 as input, both are LSB and MSB of GPIO
    //write to init
    Write_Message_I2C(ADDRESS, IODIR, 0b00000001);
    //same gppu to set button pull up (change to gppu)
    Write_Message_I2C(ADDRESS, GPPU, 0b00000001);
//    Write_Message_I2C(ADDRESS, OLAT, 0b10000000);

    while (1) {
        blink_LED();
//            Write_Message_I2C(ADDRESS, OLAT, 0b10000000);

        //read from gpio
                unsigned char gpio_status = Read_Message_I2C(ADDRESS, GPIO);
                if ((gpio_status & 0b00000001) == 0b0) {
                    Write_Message_I2C(ADDRESS, OLAT, (0b10000000));
                } else{ 
                    Write_Message_I2C(ADDRESS, OLAT, (0b00000000));                    
                }
        //check if button is pushed
        //if so, turn on b4

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
    i2c_master_send(address << 1);
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
    i2c_master_send(address << 1);
    //write register to change
    i2c_master_send(reg);
    //reset i2c
    i2c_master_restart();
    i2c_master_send(address << 1 | 0b1); //to read
    unsigned char ret = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();

    return ret;
}

void blink_LED() {
    LATAbits.LATA4 = !button_val;
    button_val = !button_val;
    int i = 0;
    for (i = 0; i < 1000000; i++) {
        ;
    }
    //    Write_Message_I2C(ADDRESS, GPIO, (!button_val<<7));
    //    Write_Message_I2C(ADDRESS, GPIO, (0b10000000));

}

/*
//questions for oh:
//    - what should init do? is the i2c init for the pic or for the ic
 * 
//    - what is final goal to show? which LED is heartbeat and which is button on and off
 * 
//    - should the blink have a delay?
 * 
//    - where do i specify which one is my sda pin
 * 
 */
