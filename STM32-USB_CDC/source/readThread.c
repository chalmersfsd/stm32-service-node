/*
 * read_write.c
 *
 *  Created on: Feb 17, 2019
 *      Author: mvshv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readThread.h"
#include "protocol.h"
#include "portab.h"
#include "usbcfg.h"

void decodeNextNetstring(char* buffer, int size);
void decodeRequest(char* msg);

THD_FUNCTION(readThrFunction, arg)
{
  (void) arg;
  chRegSetThreadName("Read thread");
  char readBuffer[128];
  event_listener_t usbData;
  eventflags_t flags;
  int bytesRead = 0;
  chEvtRegisterMask((event_source_t *)chnGetEventSource(&PORTAB_SDU1), &usbData, EVENT_MASK(1));
  while (TRUE)
  {
    chEvtWaitAll(EVENT_MASK(1));

    flags = chEvtGetAndClearFlags(&usbData);
    if (flags & CHN_INPUT_AVAILABLE)
    {
      bytesRead = chnReadTimeout(&PORTAB_SDU1, readBuffer, 64, 1000);
      if (bytesRead > 3)
      {
        decodeNextNetstring(readBuffer, bytesRead);
      }
    }
  }
}

void decodeNextNetstring(char* buffer, int size)
{
  // Netstrings have the following format:
  // ASCII Number representing the length of the payload + ':' + payload + MSG_END

  int msg_length = -1;
  char netstring[64];
  const char delim1[] = ":";
  const char delim2[] = ";";
  char *temp = NULL;
  char *temp1 = NULL;
  char* copy=  buffer;
  char** bufferPos = &copy;


  temp1 = strtok_r(copy, delim1, bufferPos);
  temp = strtok_r(NULL, delim2, bufferPos);
  msg_length = atoi(temp1);
  if (msg_length != -1 && msg_length < size)
  {
    memcpy(netstring, temp, msg_length);
    decodeRequest(netstring);
  }
}

typedef enum {
  GPIO = 0,
  PWM = 1,
} pinType;

void decodeRequest(char* msg){

  pinType type;
  unsigned int pinID = 0;
  const char delim[] = "|";
  char* copy = msg;
  char **temp = &msg;
  char* command = strtok_r(copy, delim, temp);
  char* sensor = strtok_r(NULL, delim, temp);
  char* value = strtok_r(NULL, delim, temp);
  /*
   * Process command
   */
  if (strstr(command, SET))
  {
    //We only have SET command for now, do nothing
  }

  /*
   * Process sensor
   */
  if (strstr(sensor, HEART_BEAT))
  {
    type = GPIO;
    pinID = 0;
  }
  else if (strstr(sensor, RACK_RIGHT))
  {
    type = GPIO;
    pinID = 1;
  }
  else if (strstr(sensor, RACK_LEFT))
  {
    type = GPIO;
    pinID = 2;
  }
  else if (strstr(sensor, SERVICE_BREAK))
  {
    type = GPIO;
    pinID = 3;
  }
  else if (strstr(sensor, REDUNDENCY))
  {
    type = GPIO;
    pinID = 4;
  }
  else if (strstr(sensor, REDUNDENCY))
  {
    type = GPIO;
    pinID = 6;
  }
  else if (strstr(sensor, SPARE))
  {
    type = GPIO;
    pinID = 7;
  }
  else if (strstr(sensor, CLAMP_SET))
  {
    type = GPIO;
    pinID = 8;
  }
  else if (strstr(sensor, COMPRESSOR))
  {
    type = GPIO;
    pinID = 9;
  }
  else if (strstr(sensor, EBS_RELIEF))
  {
    type = GPIO;
    pinID = 10;
  }
  else if (strstr(sensor, EBS_SPEAKER))
  {
    type = GPIO;
    pinID = 11;
  }
  else if (strstr(sensor, FINISHED))
  {
    type = GPIO;
    pinID = 12;
  }
  else if (strstr(sensor, STEER_SPEED))
  {
    type = PWM;
    pinID = 5;
  }
  else if (strstr(sensor, BRAKE_PRESSURE))
  {
    type = PWM;
    pinID = 6;
  }
  else if (strstr(sensor, ASSI_BLUE))
  {
    type = PWM;
    pinID = 7;
  }
  else if (strstr(sensor, ASSI_RED))
  {
    type = PWM;
    pinID = 8;
  }
  else if (strstr(sensor, ASSI_GREEN))
  {
    type = PWM;
    pinID = 9;
  }
  /*
   * Process value to set for sensor
   */
  int intValue = atoi(value);
  unsigned char pinValue = intValue == 0 ? PAL_LOW : PAL_HIGH;

  /*
   * Execute command
   */
  switch(type)
  {
    case GPIO:
      palWritePad(GPIOD, pinID, pinValue);
      break;
    case PWM:
      switch(pinID)
      {
        case 5: // linear actuator steer speed
          pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, intValue)); break;
        case 6: // pressure regulator
          pwmEnableChannel(&PWMD4, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, intValue)); break;
        case 7: // ASSI blue
          pwmEnableChannel(&PWMD4, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, intValue)); break;
        case 8: // ASSI red
          pwmEnableChannel(&PWMD4, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, intValue)); break;
        case 9: // ASSI green
          pwmEnableChannel(&PWMD4, 3, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, pinValue)); break;
      }
      break;
    default:
      break;
  }
}
