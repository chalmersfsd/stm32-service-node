/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "portab.h"

#include "shell.h"
#include "chprintf.h"

#include "usbcfg.h"

#include "readThread.h"
#include "writeThread.h"
#include "adcThread.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
  static uint8_t buf[] =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: write\r\n");
    return;
  }

  while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
#if 1
    /* Writing in channel mode.*/
    chnWrite(&PORTAB_SDU1, buf, sizeof buf - 1);
#else
    /* Writing in buffer mode.*/
    (void) obqGetEmptyBufferTimeout(&PORTAB_SDU1.obqueue, TIME_INFINITE);
    memcpy(PORTAB_SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
    obqPostFullBuffer(&PORTAB_SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
#endif
  }
  chprintf(chp, "\r\n\nstopped\r\n");
}

static const ShellCommand commands[] = {
  {"write", cmd_write},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&PORTAB_SDU1,
  commands
};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/
/*
 * ADC related stuff
 */
mutex_t adcSampleMutex;
adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
void ADCinit(void)
{
  palSetGroupMode(GPIOC, PAL_PORT_BIT(0), 0, PAL_MODE_INPUT_ANALOG); //Pin PC0
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1), 0, PAL_MODE_INPUT_ANALOG); //Pin PC1
  palSetGroupMode(GPIOC, PAL_PORT_BIT(2), 0, PAL_MODE_INPUT_ANALOG); //Pin PC2
  palSetGroupMode(GPIOC, PAL_PORT_BIT(3), 0, PAL_MODE_INPUT_ANALOG); //Pin PC3
  palSetGroupMode(GPIOC, PAL_PORT_BIT(4), 0, PAL_MODE_INPUT_ANALOG); //Pin PC4
  palSetGroupMode(GPIOC, PAL_PORT_BIT(5), 0, PAL_MODE_INPUT_ANALOG); //Pin PC5
  adcStart(&ADCD1, NULL);
}
/*
 * PWM related
 */
static PWMConfig pwmcfg = {
  100000,                                    /* 10kHz PWM clock frequency.   */
  10,                                    /* PWM period 100 (in ticks).    */
  NULL,
  {
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}
  },
  /* HW dependent part.*/
  0,
  0
};

void PWMInit(void)
{
  pwmStart(&PWMD4, &pwmcfg);
  pwmStart(&PWMD3, &pwmcfg);

  palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(2)); //pb5, alternate function 2 (TIM3_CH2)
  palSetPadMode(GPIOB, 6, PAL_MODE_ALTERNATE(2)); //pb5, alternate function 2 (TIM4_CH1)
  palSetPadMode(GPIOB, 7, PAL_MODE_ALTERNATE(2)); //pb5, alternate function 2 (TIM4_CH2)
  palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(2)); //pb5, alternate function 2 (TIM4_CH3)
  palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(2)); //pb5, alternate function 2 (TIM4_CH4)

  pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0));
  pwmEnableChannel(&PWMD4, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0));
  pwmEnableChannel(&PWMD4, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0));
  pwmEnableChannel(&PWMD4, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0));
  pwmEnableChannel(&PWMD4, 3, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0));
}
/*
 * Communication activity thread
 */
static THD_WORKING_AREA(usbThreadWA, 128);
static THD_FUNCTION(usbThreadFunction, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time;
    time = serusbcfg.usbp->state == USB_ACTIVE ? 50 : 5000;
    palClearLine(LINE_LED4);
    chThdSleepMilliseconds(time);
    palSetLine(LINE_LED4);
    chThdSleepMilliseconds(time);
  }
}

/*
 * Thread working areas
 */
static THD_WORKING_AREA(readThrWA, 1024);
static THD_WORKING_AREA(writeThrWA, 1024);
static THD_WORKING_AREA(adcSampleThreadWA, 64);

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Board-dependent initialization.
   */
  portab_setup();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&PORTAB_SDU1);
  sduStart(&PORTAB_SDU1, &serusbcfg);

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  /*
   * Start ADC driver
   */
  ADCinit();
  /*
   * Start PWM driver
   */
  PWMInit();

  /*
   * Shell manager initialization.
   */
//  shellInit();

  /*
   * Create threads.
   */
  chThdCreateStatic(readThrWA, sizeof(readThrWA), NORMALPRIO, readThrFunction, NULL);
  chThdCreateStatic(writeThrWA, sizeof(writeThrWA), NORMALPRIO, writeThrFunction, NULL);
  chThdCreateStatic(usbThreadWA, sizeof(usbThreadWA), NORMALPRIO, usbThreadFunction, NULL);
  chThdCreateStatic(adcSampleThreadWA, sizeof(adcSampleThreadWA), NORMALPRIO, adcSampleThread, NULL);

  /*
   * Normal main() thread activity, spawning shells.
   */
  while (true) {
//    if (PORTAB_SDU1.config->usbp->state == USB_ACTIVE) {
//      thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
//                                              "shell", NORMALPRIO + 1,
//                                              shellThread, (void *)&shell_cfg1);
//      chThdWait(shelltp);               /* Waiting termination.             */
//    }
    chThdSleepMilliseconds(10);
  }
}
