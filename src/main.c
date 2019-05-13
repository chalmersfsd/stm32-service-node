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

#include "chprintf.h"

#include "usbcfg.h"

#include "communicationThread.h"
#include "adcThread.h"

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
  pwmStart(&PWMD3, &pwmcfg);
  pwmStart(&PWMD4, &pwmcfg);

  pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0)); // ASSI_RED
  pwmEnableChannel(&PWMD3, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0)); // LINEAR ACTUATOR
  pwmEnableChannel(&PWMD3, 3, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0)); // PRESSURE
  pwmEnableChannel(&PWMD4, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0)); // ASSI_GREEN
  pwmEnableChannel(&PWMD4, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, 0)); // ASSI_BLUE
}
/*
 * CAN related part
 */
struct can_instance {
  CANDriver     *canp;
  uint32_t      led;
};

static const struct can_instance can1 = {&CAND1, GPIOD_LED3};

/*
 * 500KBaud, automatic wakeup, automatic recover from abort mode.
 * See section 22.7.7 on the STM32 reference manual.
 */
static const CANConfig cancfg = {
  CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
  CAN_BTR_SJW(0) | CAN_BTR_TS2(1) |
  CAN_BTR_TS1(8) | CAN_BTR_BRP(6)
};

static THD_FUNCTION(can_rx, p) {
  struct can_instance *cip = p;
  event_listener_t el;
  CANRxFrame rxmsg;

  (void)p;
  chRegSetThreadName("receiver");
  chEvtRegister(&cip->canp->rxfull_event, &el, 0);
  while (true) {
    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0)
      continue;
    while (canReceive(cip->canp, CAN_ANY_MAILBOX,
                      &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
      /* Process message.*/
      palTogglePad(GPIOD, cip->led);
    }
  }
  chEvtUnregister(&CAND1.rxfull_event, &el);
}

static THD_FUNCTION(can_tx, p) {
  CANTxFrame txmsg;

  (void)p;
  chRegSetThreadName("transmitter");
  txmsg.IDE = CAN_IDE_EXT;
  txmsg.EID = 0x01234567;
  txmsg.RTR = CAN_RTR_DATA;
  txmsg.DLC = 8;
  txmsg.data32[0] = 0x55AA55AA;
  txmsg.data32[1] = 0x00FF00FF;

  while (true) {
    canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));
    chThdSleepMilliseconds(50);
  }
}

/*
 * Thread working areas
 */
static THD_WORKING_AREA(can_rx1_wa, 256);
static THD_WORKING_AREA(can_tx_wa, 256);
static THD_WORKING_AREA(usbThreadWA, 64);
static THD_WORKING_AREA(communicationThreadWA, 1024);
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
   * Start drivers
   */
  ADCinit();
  PWMInit();
  canStart(&CAND1, &cancfg);

  /*
   * Starting the transmitter and receiver threads.
   */
  chThdCreateStatic(can_rx1_wa, sizeof(can_rx1_wa), HIGHPRIO,
                    can_rx, (void *)&can1);
  chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), HIGHPRIO,
                    can_tx, NULL);

  /*
   * Create threads.
   */
  chThdCreateStatic(communicationThreadWA, sizeof(communicationThreadWA), NORMALPRIO,
                    communicationThrFunction, NULL);
  chThdCreateStatic(usbThreadWA, sizeof(usbThreadWA), NORMALPRIO,
                    usbThreadFunction, NULL);
  chThdCreateStatic(adcSampleThreadWA, sizeof(adcSampleThreadWA), NORMALPRIO,
                    adcSampleThread, NULL);

  /*
   * Normal main() thread activity.
   */
  while (true) {
    chThdSleepMicroseconds(1);
  }
  return 0;
}
