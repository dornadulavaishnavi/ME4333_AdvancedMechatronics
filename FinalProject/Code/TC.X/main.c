#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ssd1306.h"
#include "i2c_master_noint.h"
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
#pragma config WDTPS = PS1048576  // use largest wdt value
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

#define PIC32_SYS_FREQ 48000000ul // 48 million Hz
#define PIC32_DESIRED_BAUD 230400 // Baudrate for RS232
#define P_GAIN 30000

void PIC_INIT();
void init_Timer3();
void init_Timer2();
void UART2_Startup(void);
void ReadUART2(char * string, int maxLength);
void WriteUART2(const char * string);

void main() {
    PIC_INIT();
    init_Timer3();
    init_Timer2();
    UART2_Startup();
    ssd1306_setup();

    char message[30];
    ssd1306_clear();
    sprintf(message, "No message yet");
    ssd1306_DrawString(5, 24, message);
    ssd1306_update();

    while (1) {
        char m[25];
        ReadUART2(m, 25); // get the number from the Pico
        float pos = 0;
        sscanf(m, "%f", &pos);

        ssd1306_clear();
        ssd1306_DrawString(5, 24, m);
        ssd1306_update();

        float u = pos*P_GAIN;
        if (u > 999.0) {
            u = 999.0;
        }
        if (u < -999.0) {
            u = -999.0;
        }
        WriteUART2(m);

        //        u = 999;
        if (pos<-2) {
            OC1RS = (999 + u); //1999 is max, red wheel
            LATBbits.LATB14 = 1;
            //            OC4RS = (999 - u); //1999 is max, black wheel
            //            LATBbits.LATB12 = 1;
        } else if (pos > 2) {
            //            OC1RS = (999 - u); //1999 is max, red wheel
            //            LATBbits.LATB14 = 1;
            OC4RS = (999 + u); //1999 is max, black wheel
            LATBbits.LATB12 = 1;
        } else {
            OC1RS = (999 + u); //1999 is max, red wheel
            LATBbits.LATB14 = 1;
            OC4RS = (999 + u); //1999 is max, black wheel
            LATBbits.LATB12 = 1;
        }
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

    TRISBbits.TRISB12 = 0;
    LATBbits.LATB12 = 0;
    TRISBbits.TRISB13 = 0;
    LATBbits.LATB13 = 0;
    TRISBbits.TRISB14 = 0;
    LATBbits.LATB14 = 0;
    TRISBbits.TRISB15 = 0;
    LATBbits.LATB15 = 0;

    RPB15Rbits.RPB15R = 0b0101; //set B15 pin to correspond with OC1
    RPB13Rbits.RPB13R = 0b0101; //set B13 pin to correspond with OC4

    // UART to the Pico
    U2RXRbits.U2RXR = 0b0011; // Set B11 to U2RX
    RPB10Rbits.RPB10R = 0b0010; // Set B10 to U2TX

}

void init_Timer3() {
    T3CONbits.TCKPS = 1; //Timer 3 Prescaler N = 2
    OC1CONbits.OCTSEL = 1; //set oc1 to use timer 3
    PR3 = 1999; //setting up to run 20kHz
    TMR3 = 0; //initialize TMR3 count to 0
    OC1CONbits.OCM = 0b110; //PWM mode without fault pin
    OC1RS = 0; //Duty Cycle = OC1RS/(PR3 + 1) = 75%
    OC1R = 0; //initialize before turning OC1 on
    T3CONbits.ON = 1;
    OC1CONbits.ON = 1;
}

void init_Timer2() {
    T2CONbits.TCKPS = 1; //Timer 2 Prescaler N = 2
    OC4CONbits.OCTSEL = 0; //set oc4 to use timer 2
    PR2 = 1999; //setting up to run 20kHz
    TMR2 = 0; //initialize TMR2 count to 0
    OC4CONbits.OCM = 0b110; //PWM mode without fault pin
    OC4RS = 0; //Duty Cycle = OC4RS/(PR2 + 1) = 75%
    OC4R = 0; //initialize before turning OC4 on
    T2CONbits.ON = 1;
    OC4CONbits.ON = 1;
}

void UART2_Startup() {
    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // turn on UART1 without an interrupt
    U2MODEbits.BRGH = 0; // set baud to PIC32_DESIRED_BAUD
    U2BRG = ((PIC32_SYS_FREQ / PIC32_DESIRED_BAUD) / 16) - 1;

    // 8 bit, no parity bit, and 1 stop bit (8N1 setup)
    U2MODEbits.PDSEL = 0;
    U2MODEbits.STSEL = 0;

    // configure TX & RX pins as output & input pins
    U2STAbits.UTXEN = 1;
    U2STAbits.URXEN = 1;

    // enable the uart
    U2MODEbits.ON = 1;
    __builtin_enable_interrupts();

}

// Read from UART1
// block other functions until you get a '\r' or '\n'
// send the pointer to your char array and the number of elements in the array

void ReadUART2(char * message, int maxLength) {
    char data = 0;
    int complete = 0, num_bytes = 0;
    // loop until you get a '\r' or '\n'
    while (!complete) {
        if (U2STAbits.URXDA) { // if data is available
            data = U2RXREG; // read the data
            if ((data == '\n') || (data == '\r')) {
                complete = 1;
            } else {
                message[num_bytes] = data;
                ++num_bytes;
                // roll over if the array is too small
                if (num_bytes >= maxLength) {
                    num_bytes = 0;
                }
            }
        }
    }
    // end the string
    message[num_bytes] = '\0';
}

// Write a character array using UART1

void WriteUART2(const char * string) {
    while (*string != '\0') {
        while (U2STAbits.UTXBF) {
            ; // wait until tx buffer isn't full
        }
        U2TXREG = *string;
        ++string;
    }
}