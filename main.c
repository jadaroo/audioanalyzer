#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/regs/dreq.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/dma.h>
#include <math.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "dft.h"
#include "initialization.h"

#define BUFFER_SIZE 256
#define CLKDIV 999
#define TEMP_SENSOR_ADC 4
#define SLEEP_BEFORE_START_MS 3000
#define DISPLAY_LENGTH 128

uint FIRST_CHANNEL = 0;
uint SECOND_CHANNEL = 1;

float test_sin[256];
float test_cos[256];

uint16_t first_array[BUFFER_SIZE];
uint16_t second_array[BUFFER_SIZE];

float mag[DISPLAY_LENGTH];
int32_t waves_buffer[BUFFER_SIZE];

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


int main(){  
  stdio_init_all();

  init_adc_with_fifo(TEMP_SENSOR_ADC, CLKDIV);
  init_dma_with_chaining(&FIRST_CHANNEL, &SECOND_CHANNEL, first_array, second_array, BUFFER_SIZE);
  
  sleep_ms(SLEEP_BEFORE_START_MS);

  dma_start_channel_mask(1u << FIRST_CHANNEL);
  
  printf("Starting...\n");
  adc_run(true);
  
  while(true){    
    if(first_array_ready){
      process_buffer(first_array, waves_buffer, BUFFER_SIZE, DISPLAY_LENGTH, test_sin, test_cos, mag);
      first_array_ready = false;
    } 

    if(second_array_ready){
      process_buffer(second_array, waves_buffer, BUFFER_SIZE, DISPLAY_LENGTH, test_sin, test_cos, mag);
      second_array_ready = false;
    }

    for(uint8_t i = 0; i < DISPLAY_LENGTH; i++){
      printf("%f, ", mag[i]);
    }
    printf("\n");
  }
}