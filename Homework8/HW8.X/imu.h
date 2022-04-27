#define NUM_DATA_POINTS 200
#define A 0.1
#define dt 24000000/2/20

void comp_filter(uint8_t *imu_buf, float *pitch, float *roll);