#include "dft.h"

#include <stdint.h>
#include <math.h>

void set_sin_tests(float *sin_tests, uint16_t buffer_size){
  for(uint16_t i = 0; i < buffer_size; i++){
    sin_tests[i] = sinf(2 * M_PI * i / buffer_size);
  }
}

void set_cos_tests(float *cos_tests, uint16_t buffer_size){
  for(uint16_t i = 0; i < buffer_size; i++){
    cos_tests[i] = cosf(2 * M_PI * i / buffer_size);
  }
}

uint16_t calculate_mid(uint16_t *sample_buffer, uint16_t buffer_size){
  uint32_t mid = 0;
  for(uint16_t i = 0; i < buffer_size; i++){
    mid += sample_buffer[i];
  }

  return mid / buffer_size;
}

void process_buffer(uint16_t *array, int32_t *waves_buffer, uint16_t buffer_size, uint8_t display_length, float *test_cos, float *test_sin, float *mag){
  uint16_t mid = calculate_mid(array, buffer_size);
  for(uint16_t i = 0; i < buffer_size; i++){
    waves_buffer[i] = (int32_t)array[i] - mid;
  }

  for(uint8_t j = 0; j < display_length; j++){
    uint16_t test_pos = 0;
    float co = 0;
    float si = 0;
    for(uint16_t f = 0; f < buffer_size; f++){
      test_pos %= buffer_size;
      co += waves_buffer[f] * test_cos[test_pos];
      si += waves_buffer[f] * test_sin[test_pos];
      test_pos += j;
    }

    mag[j] = sqrtf(co * co + si * si);
  }
}