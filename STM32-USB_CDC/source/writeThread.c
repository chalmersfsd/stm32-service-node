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

THD_FUNCTION(writeThrFunction, arg)
{
  (void) arg;
  event_listener_t usbData;
  eventflags_t flags;
  int bytesRead = 0;
  chEvtRegisterMask((event_source_t *)chnGetEventSource(&PORTAB_SDU1), &usbData, EVENT_MASK(1));

  chRegSetThreadName("write thread");
  char payloadBuffer[256];
  char temp[64];
  size_t payloadLength = 0;
  size_t msgLength = 0;
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

    payloadLength = 0;
    msgLength = sprintf(temp, "status|ebs_line|%ld", raw0);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|ebs_actuator|%ld", raw1);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|pressure_rag|%ld", raw2);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|service_tank|%ld", raw3);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|position_rack|%ld", raw4);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|steer_pos|%ld", raw5);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|asms|%d", raw6);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|clamped_sensor|%d", raw7);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);
    msgLength = sprintf(temp, "status|ebs_ok|%d", raw8);
    payloadLength += sprintf(payloadBuffer+payloadLength, "%d:%s;", msgLength, temp);

//    flags = chEvtGetAndClearFlags(&usbData);
//    if (flags & CHN_OUTPUT_EMPTY)
//    {
    chnWrite(&PORTAB_SDU1, payloadBuffer, payloadLength);
//    }
    chEvtWaitAny(EVENT_MASK(1));

    chThdSleepMilliseconds(10);
  }
}
