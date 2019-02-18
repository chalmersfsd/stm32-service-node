/*
 * writeThread.c
 *
 *  Created on: Feb 18, 2019
 *      Author: mvshv
 */

#include <stdio.h>
#include <string.h>
#include "writeThread.h"
#include "portab.h"
#include "usbcfg.h"


extern adcsample_t samples[];

void writeAnalog(char* payloadBuffer, int payloadLength)
{
  char writeBuffer[64];
  int bytesToWrite = sprintf(writeBuffer, "%d:%s;", payloadLength, payloadBuffer);
  chnWriteTimeout(&PORTAB_SDU1, writeBuffer, bytesToWrite, 10);
}

THD_FUNCTION(writeThrFunction, arg)
{
  (void) arg;
  chRegSetThreadName("write thread");
  char payloadBuffer[64];
  int payloadLength = 0;
  uint32_t raw0 = 0;
  uint32_t raw1 = 0;
  uint32_t raw2 = 0;
  uint32_t raw3 = 0;
  uint32_t raw4 = 0;
  uint32_t raw5 = 0;
  bool raw6 = false;
  bool raw7 = false;
  bool raw8 = false;
  while (TRUE)
  {
    //Analog input
    raw0 = (uint32_t)(samples[0] + samples[6] + samples[12] + samples[18]) / 4; //PC0
    raw1 = (uint32_t)(samples[1] + samples[7] + samples[13] + samples[19]) / 4; //PC1
    raw2 = (uint32_t)(samples[2] + samples[8] + samples[14] + samples[20]) / 4; //PC2
    raw3 = (uint32_t)(samples[3] + samples[9] + samples[15] + samples[21]) / 4; //PC3
    raw4 = (uint32_t)(samples[4] + samples[10] + samples[16] + samples[22]) / 4; //PC4
    raw5 = (uint32_t)(samples[5] + samples[11] + samples[17] + samples[23]) / 4; //PC5
    //Digital input
    raw6 = palReadPad(GPIOC, 13); //PC13
    raw7 = palReadPad(GPIOC, 14); //PC14
    raw8 = palReadPad(GPIOC, 15); //PC15

    payloadLength = sprintf(payloadBuffer, "status|ebs_line|%ld", raw0);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|ebs_actuator|%ld", raw1);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|pressure_rag|%ld", raw2);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|service_tank|%ld", raw3);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|position_rack|%ld", raw4);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|steer_pos|%ld", raw5);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|asms|%d", raw6);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|clamped_sensor|%d", raw7);
    writeAnalog(payloadBuffer, payloadLength);
    payloadLength = sprintf(payloadBuffer, "status|ebs_ok|%d", raw8);
    writeAnalog(payloadBuffer, payloadLength);

    chThdSleepMilliseconds(10);
  }
}
