/*
 * adcThread.c
 *
 *  Created on: Feb 18, 2019
 *      Author: mvshv
 */

#include "adcThread.h"

extern adcsample_t samples[];

static const ADCConversionGroup adcgrpcfg =
{
  FALSE, // circular buffer mode
  ADC_GRP1_NUM_CHANNELS, // number of the analog channels
  NULL, // callback function
  NULL, // error callback
  0, // CR1
  ADC_CR2_SWSTART, // CR2
  ADC_SMPR1_SMP_AN10(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN12(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN13(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN14(ADC_SAMPLE_3) | ADC_SMPR1_SMP_AN15(ADC_SAMPLE_3),// sample times for channel 10-18
  0,// sample times for channel 0-9
  ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),// ADC SQR1 Conversion group sequence 13-16 + sequence length.
  0, // ADC SQR2 Conversion group sequence 7-12
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN10) | ADC_SQR3_SQ2_N(ADC_CHANNEL_IN11) | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN12) | ADC_SQR3_SQ4_N(ADC_CHANNEL_IN13) | ADC_SQR3_SQ5_N(ADC_CHANNEL_IN14) | ADC_SQR3_SQ6_N(ADC_CHANNEL_IN15) // ADC SQR3 Conversion group sequence 1-6
};

THD_FUNCTION(adcSampleThread, arg)
{
  (void) arg;
  chRegSetThreadName("Sample ADC");
  while (true)
  {
    adcConvert(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
  }
}
