#include <stdint.h>
#ifndef DFT_H

void set_sin_tests(float *sin_tests, uint16_t buffer_size);
void set_cos_tests(float *cos_tests, uint16_t buffer_size);
uint16_t calculate_mid(uint16_t *sample_buffer, uint16_t buffer_size);
void process_buffer(uint16_t *array, int32_t *waves_buffer, uint16_t buffer_size, uint8_t display_length, float *test_cos, float *test_sin, float *mag);

#endif