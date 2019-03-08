/*
 * adcThread.h
 *
 *  Created on: Feb 18, 2019
 *      Author: mvshv
 */

#ifndef SOURCE_ADCTHREAD_H_
#define SOURCE_ADCTHREAD_H_

#include "ch.h"
#include "hal.h"

#define ADC_GRP1_NUM_CHANNELS   6
#define ADC_GRP1_BUF_DEPTH      4

THD_FUNCTION(adcSampleThread, arg);

#endif /* SOURCE_ADCTHREAD_H_ */
