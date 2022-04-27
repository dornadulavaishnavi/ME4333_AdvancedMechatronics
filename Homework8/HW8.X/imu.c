#include <math.h>
#include "mpu6050.h"
#include "imu.h"
#include "UART.h"

char m_out[200];

void comp_filter(uint8_t *imu_buf, float *pitch, float *roll) {
    float ax, ay, az, gx, gy, gz, temp;
    float theta_accel, phi_accel;

    ax = conv_xXL(imu_buf);
    ay = conv_yXL(imu_buf);
    az = conv_zXL(imu_buf);
    gx = conv_xG(imu_buf);
    gy = conv_yG(imu_buf);
    gz = conv_zG(imu_buf);
    temp = conv_temp(imu_buf);

    theta_accel = atan2f(ax, az);
    phi_accel = atan2f(ay, az);

    //    *pitch = theta_accel;
    //    *roll = phi_accel;

    *pitch += gx * dt;
    *pitch = A * theta_accel + (1 - A)*(*pitch);
    *roll += gy * dt;
    *roll = A * phi_accel + (1 - A)*(*roll);

    sprintf(m_out, "%d %d %d\r\n", ax, ay, az);
    NU32_WriteUART1(m_out);
}
