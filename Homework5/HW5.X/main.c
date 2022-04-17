#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include "spi.h"
#include "UART.h"

void PIC_INIT();
unsigned short Convertto16bit(char channel, unsigned char v);

void main() {
    //keep pic init, A4, B4, UART
    PIC_INIT();
    UART1_INIT();
    //initialize pins and write to output
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 0;

    //init spi
    initSPI();
    float degree = 0;
    float flag_direction = 1.0;
    while (1) {
        //    output 2hz sin and 1Hz triangle
        //    convert from short to 2 unsigned chars
//        if(degree >= (2*M_PI)){
//            degree = 2*M_PI;
//            flag_direction = -1.0;
//        } else if(degree <= 0){
//            degree = 0;
//            flag_direction = 1.0;
//        }
        unsigned char out_1;
        unsigned char out_2;
        unsigned short output;
        unsigned char volt = 0b11111111 * ((sin(degree)/2.1)+127.0); //just added sin portion to go from triangle to sin and had just the if degree is greater, reset to 0
        output = Convertto16bit(0, volt);
        unsigned char in_1 = output >> 8;
        unsigned char in_2 = (output & 0b0000000011111111);
        //        unsigned char in_1 = 0b01111111;
        //        unsigned char in_2 = 0b11110000;
        //    update VoutA
        //    talking to DAC
        //    CS goes low
        //    lat pin low (0)
        LATBbits.LATB6 = 0;
        //    send 16bits 
        //    call spi_io twice with 8 bit # parameter        
        out_1 = spi_io(in_1);
        out_2 = spi_io(in_2);
        //    CS goes high
        //    lat pin high (1)
        LATBbits.LATB6 = 1;

        //    repeat to send VoutB
        //        LATBbits.LATB6 = 0;
        //        out_2 = spi_io(in_2);
        //        LATBbits.LATB6 = 1;

        //    16 bit number is a or b, 1, 1, 1, 8bits of voltage, last 4 dont matter (split into 8 and 8 to send first half then second) (pg24 of IC data sheet)
        //    unsigned chars for 8bit number
        //    shorts are 16bit numbers (s = (c1<<8)|c2)

        //    for sin, do scalar to sin or array with values
        degree += (0.95*flag_direction);
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
}

unsigned short Convertto16bit(char channel, unsigned char voltage) {
    unsigned short s = 0;

    s = s | (channel << 15);
    s = s | (0b111 << 12);
    s = s | (voltage << 4);

    return s;
}