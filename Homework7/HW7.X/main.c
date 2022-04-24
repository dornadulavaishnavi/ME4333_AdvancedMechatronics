#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <math.h>
#include "mpu6050.h"
#include "UART.h"
#include "i2c_master_noint.h"
#include "imu.h"

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
void blink();
unsigned short Convertto16bit(char channel, unsigned char v);

//global vars
int button_val = 0;

void main() {
    //keep pic init, A4, B4, UART
    PIC_INIT();
    UART1_INIT();
    //below done in init_mpu6050
    //turn on the chip by writing to PWR_MGMT
    //turn on accelerometer by writing to ACCEL_CONFIG and change sensitivity to +- 2g
    //turn on gyroscope by writing to GYRO_CONFIG and set sensitivity to +- 2000 dps
    init_mpu6050(); //inits i2c

    char m_in[100]; // char array for uart data coming in
    char m_out[200]; // char array for uart data going out
    uint8_t imu_buf[NUM_DATA_POINTS];
    int i;
    float pitch = 0;
    float roll = 0;

    //check that who am 1 register contains 0x68
    //if wrong value, go into infinite while loop with led on to indicate power reset
    char who = whoami(); // ask if the imu is there
    if (who != 0x68) {
        // if the imu is not there, get stuck here forever
        while (1) {
            LATAbits.LATA4 = 1;
        }
    }

    while (1) {
        //add heartbeat
        blink();

        NU32_ReadUART1(m_in, 100); // wait for a newline
        // don't actually have to use what is in m
        
        blink();
        // collect data
        for (i = 0; i < NUM_DATA_POINTS; i++) {
            _CP0_SET_COUNT(0);
            //use burst_read_I2C1() to read all data from chip
            burst_read_mpu6050(imu_buf);
            
            //complementary filter process the pitch and roll
            comp_filter(imu_buf, &pitch, &roll);
            
            //output (sprintf and write to uart)
            sprintf(m_out,"%d %f %f\r\n",i, pitch, roll);
            NU32_WriteUART1(m_out);
            
            //delay by dt
            while (_CP0_GET_COUNT() < dt) {
            }
        }
        blink();
        
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

//Void comp_filter(uint8_t *imu_buf, float *pitch, float *roll
//Convert that array data to gs and dps(impu_buf)
//Ax = conv_x_accel(imu_buf)
//Should be ~1 or -1 when lying flat
//Repeat for y and z
//Repeat for gyro_x and y
//Do trig to get pitch and roll using atan2f from accel data
//Pitch is a ?return? but pointer because also need to ?return? roll
//*pitch += gyro_x * dt
//*pitch = A*theta_accel + (1-A)*pitch
//Repeat d and e for roll




