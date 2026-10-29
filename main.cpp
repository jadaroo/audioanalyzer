#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <hardware/regs/dreq.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/dma.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "i2c_bus/i2c_transport.hpp"
#include "math.h"

#include "dft.h"
#include "initialization.h"
#include "display/display.hpp"

#define BUFFER_SIZE 256
#define CLKDIV 999
#define SLEEP_BEFORE_START_MS 3000
#define DISPLAY_LENGTH 128
#define DISPLAY_HEIGHT 64
#define FLOOR_DB 42.0f
#define RANGE_DB 48.0f
#define DISPLAY_POWER_UP_MS 200
#define BAR_COUNT 32
#define BAR_WIDTH 3
#define BAR_STEP 4

#define DISPLAY_ADDRESS 0x3C

uint FIRST_CHANNEL = 0;
uint SECOND_CHANNEL = 1;

float test_sin[BUFFER_SIZE];
float test_cos[BUFFER_SIZE];

uint16_t first_array[BUFFER_SIZE];
uint16_t second_array[BUFFER_SIZE];

float mag[DISPLAY_LENGTH];
int32_t waves_buffer[BUFFER_SIZE];
uint8_t height[BAR_COUNT];
uint8_t bar_first_bin[BAR_COUNT + 1];

volatile bool first_array_ready = false;
volatile bool second_array_ready = false;

void dma_handler(){
  if(dma_hw->ints0 & (1u << FIRST_CHANNEL)){
    dma_channel_set_write_addr(FIRST_CHANNEL, &first_array[0], false);
    first_array_ready = true;
    dma_hw->ints0 = 1u << FIRST_CHANNEL;
  }

  if(dma_hw->ints0 & (1u << SECOND_CHANNEL)){
    dma_channel_set_write_addr(SECOND_CHANNEL, &second_array[0], false);
    second_array_ready = true;
    dma_hw->ints0 = 1u << SECOND_CHANNEL;
  }
}

void set_bar_edges(uint8_t *first_bin, uint8_t bar_count, uint8_t display_length){
  first_bin[0] = 1;
  for(uint8_t b = 1; b <= bar_count; b++){
    uint8_t edge = (uint8_t)roundf(powf(display_length, (float)b / bar_count));
    if(edge <= first_bin[b - 1]){
      edge = first_bin[b - 1] + 1;
    }
    first_bin[b] = edge;
  }
  first_bin[bar_count] = display_length;
}

int main(){
  stdio_init_all();

  init_i2c_default();
  init_adc_0_with_fifo(CLKDIV);
  init_dma_with_chaining(&FIRST_CHANNEL, &SECOND_CHANNEL, first_array, second_array, BUFFER_SIZE);

  set_sin_tests(test_sin, BUFFER_SIZE);
  set_cos_tests(test_cos, BUFFER_SIZE);
  set_bar_edges(bar_first_bin, BAR_COUNT, DISPLAY_LENGTH);

  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  sleep_ms(SLEEP_BEFORE_START_MS);

  dma_start_channel_mask(1u << FIRST_CHANNEL);

  printf("Starting...\n");
  adc_run(true);

  I2CTRANSPORT i2c(i2c_default, DISPLAY_ADDRESS, 100 * 1000);
  DISPLAY display(&i2c);

  sleep_ms(DISPLAY_POWER_UP_MS);
  display.init_disp();

  while(true){
    bool new_frame = false;

    if(first_array_ready){
      process_buffer(first_array, waves_buffer, BUFFER_SIZE, DISPLAY_LENGTH, test_cos, test_sin, mag);
      new_frame = true;
    } else if(second_array_ready){
      process_buffer(second_array, waves_buffer, BUFFER_SIZE, DISPLAY_LENGTH, test_cos, test_sin, mag);
      new_frame = true;
    }

    if(!new_frame){
      continue;
    }

    for(uint8_t b = 0; b < BAR_COUNT; b++){
      float power = 0;
      uint8_t bin_count = bar_first_bin[b + 1] - bar_first_bin[b];
      for(uint8_t i = bar_first_bin[b]; i < bar_first_bin[b + 1]; i++){
        power += mag[i] * mag[i];
      }
      power /= bin_count;

      float db = 10.0f * log10f(power + 1.0f);
      int h = (int)((db - FLOOR_DB) * DISPLAY_HEIGHT / RANGE_DB);
      if(h < 0){
        h = 0;
      }
      
      if(h > DISPLAY_HEIGHT){
        h = DISPLAY_HEIGHT;
      }

      if(h > height[b]){
        height[b] = h;
      } else if(height[b] > 0){
        height[b]--;
      }

      for(int16_t x = b * BAR_STEP; x < b * BAR_STEP + BAR_WIDTH; x++){
        for(int16_t j = 0; j < DISPLAY_HEIGHT; j++){
          if(j < height[b]){
            display.set_pixel({x, DISPLAY_HEIGHT - 1 - j});
          } else {
            display.clear_pixel({x, DISPLAY_HEIGHT - 1 - j});
          }
        }
      }
    }

    display.update_display();

    first_array_ready = false;
    second_array_ready = false;
  }
}
