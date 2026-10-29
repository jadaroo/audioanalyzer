#ifndef INITIALIZATION_H
#include <stdint.h>
#include <pico/types.h>

void init_dma_with_chaining(uint *ch1, uint *ch2, uint16_t *first_array, uint16_t *second_array, uint16_t buffer_size);
void init_adc_with_fifo(uint8_t input, uint16_t clkdiv);

#endif