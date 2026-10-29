#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include <stdint.h>
#include <pico/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_dma_with_chaining(uint *ch1, uint *ch2, uint16_t *first_array, uint16_t *second_array, uint16_t buffer_size);
void init_adc_0_with_fifo(uint16_t clkdiv);
void init_i2c_default();

#ifdef __cplusplus
}
#endif

#endif
