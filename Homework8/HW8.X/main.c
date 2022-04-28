#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include "mpu6050.h"
#include "UART.h"
#include "i2c_master_noint.h"
#include "imu.h"
#include "ssd1306.h"
#include "font.h"

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

#define NUM_DATA_PNTS 400 // how many data points to collect at 100Hz

void PIC_INIT();
void blink();
unsigned short Convertto16bit(char channel, unsigned char v);

//global vars
int button_val = 0;

void main() {
    //keep pic init, A4, B4, UART
    PIC_INIT();
    UART1_INIT();
    i2c_master_setup();
    ssd1306_setup();
    
    ssd1306_clear();
    ssd1306_update();
        
    char test = 'b';
    float i = 0.0;

    while (1) {
        //add heartbeat
        blink();    //ensure that program is not held
        
        //check if one pixel can turn on first
//        ssd1306_drawPixel(0, 0, 1);
//        ssd1306_DrawChar(0, 0, test);
        char m[50];
        char fps[50];
        sprintf(m, "Hello World");    //make message to print
        sprintf(fps, "%f", i);
        _CP0_SET_COUNT(0);
        ssd1306_DrawString(0,0,m);
        ssd1306_DrawString(0,10,fps);
        ssd1306_update();
        // collect data        
        i = 1200000.0/_CP0_GET_COUNT(); //frames per second calculated by seeing is 1 frame takes cpu get count then how many frames in 1200000 cpu counts which is 1 second
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

void blink() {
    LATAbits.LATA4 = 1;
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < dt) {
    }
    LATAbits.LATA4 = 0;
    _CP0_SET_COUNT(0);
    while (_CP0_GET_COUNT() < dt) {
    }
}



