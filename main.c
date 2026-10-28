#include <hardware/adc.h>
#include <hardware/dma.h>
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

#define BUFFER_SIZE 256
#define FIRST_CHANNEL 0
#define SECOND_CHANNEL 1
#define CLKDIV 999
#define TEMP_SENSOR_ADC 4
#define SLEEP_BEFORE_START_MS 3000

uint16_t first_array[BUFFER_SIZE];
uint16_t second_array[BUFFER_SIZE];

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

void init_adc(uint8_t input){
  adc_init();
  adc_set_temp_sensor_enabled(true);
  adc_select_input(input);

  adc_fifo_setup(
    true,
    true,
    1,
    false,
    false
  );

  adc_set_clkdiv(CLKDIV);
}


void init_dma(){
  dma_channel_claim(FIRST_CHANNEL);
  dma_channel_claim(SECOND_CHANNEL);
  
  dma_channel_config first_chan_config = dma_channel_get_default_config(FIRST_CHANNEL);
  channel_config_set_transfer_data_size(&first_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&first_chan_config, false);
  channel_config_set_write_increment(&first_chan_config, true);
  channel_config_set_chain_to(&first_chan_config, SECOND_CHANNEL);
  
  dma_channel_config second_chan_config = dma_channel_get_default_config(SECOND_CHANNEL);
  channel_config_set_transfer_data_size(&second_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&second_chan_config, false);
  channel_config_set_write_increment(&second_chan_config, true);
  channel_config_set_chain_to(&second_chan_config, FIRST_CHANNEL);

  channel_config_set_dreq(&first_chan_config, DREQ_ADC);
  channel_config_set_dreq(&second_chan_config, DREQ_ADC);
  
  dma_channel_set_irq0_enabled(FIRST_CHANNEL, true);
  dma_channel_set_irq0_enabled(SECOND_CHANNEL, true);
  
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  dma_channel_configure(
    FIRST_CHANNEL,
    &first_chan_config,
    first_array,
    &adc_hw->fifo,
    BUFFER_SIZE,
    false
  );

  dma_channel_configure(
    SECOND_CHANNEL,
    &second_chan_config,
    second_array,
    &adc_hw->fifo,
    BUFFER_SIZE,
    false
  );
}

int main(){  
  stdio_init_all();
  sleep_ms(SLEEP_BEFORE_START_MS);

  init_adc(TEMP_SENSOR_ADC);
  init_dma();

  dma_start_channel_mask(1u << FIRST_CHANNEL);
  
  printf("Starting...\n");
  adc_run(true);
  
  while(true){
    if(first_array_ready){
      uint64_t average_adc = 0;
      for(uint16_t i = 0; i < BUFFER_SIZE; i++){
        average_adc += first_array[i];
      }
      average_adc = average_adc / BUFFER_SIZE;
      printf("Average ADC read for first buffer is: %llu\n", average_adc);
      first_array_ready = false;
    } 

    if(second_array_ready){
      uint64_t average_adc = 0;
      for(uint16_t i = 0; i < BUFFER_SIZE; i++){
        average_adc += second_array[i];
      }
      average_adc = average_adc / BUFFER_SIZE;
      printf("Average ADC read for second buffer is: %llu\n", average_adc);
      second_array_ready = false;
    }
  }
}