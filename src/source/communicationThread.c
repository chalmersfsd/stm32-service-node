/*
 * communictionThread.c
 *
 *  Created on: Feb 17, 2019
 *      Author: Max Shvetsov
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "communicationThread.h"
#include "protocol.h"
#include "portab.h"
#include "usbcfg.h"

void decodeMessage(char* buffer, int size);
void decodeNetstring(char* msg);
size_t processStatus(void);
static char writeBuffer[256];
static char readBuffer[128];
extern adcsample_t samples[];

THD_FUNCTION(usbThreadFunction, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  event_listener_t usbData;
  eventflags_t flags;
  chEvtRegisterMask((event_source_t *)chnGetEventSource(&PORTAB_SDU1), &usbData, EVENT_MASK(1));
  while (true) {
    chEvtWaitAny(EVENT_MASK(1));
    flags = chEvtGetAndClearFlags(&usbData);
    if (flags & CHN_INPUT_AVAILABLE)
    {
      palSetLine(LINE_LED5);
    }
    if (flags & CHN_OUTPUT_EMPTY)
    {
      palSetLine(LINE_LED6);
    }
    chThdSleepMilliseconds(50);
    palClearLine(LINE_LED5);
    palClearLine(LINE_LED6);
    chThdSleepMilliseconds(50);
  }
}

THD_FUNCTION(communicationThrFunction, arg)
{
  (void) arg;
  chRegSetThreadName("Read thread");
  int bytesRead = 0;
  while (TRUE)
  {
    bytesRead = chnReadTimeout(&PORTAB_SDU1, (uint8_t*)readBuffer, sizeof(readBuffer), 10);
    if (bytesRead > 3)
    {
      decodeMessage(readBuffer, bytesRead);
      memset(readBuffer, 0, sizeof(readBuffer));
      memset(writeBuffer, 0, sizeof(writeBuffer));
    }
  }
}

void decodeMessage(char* buffer, int size)
{
  // Netstrings have the following format:
  // ASCII Number representing the length of the payload + ':' + payload + MSG_END

  int msg_length = -1;

  const char delim1[] = STATUS_DELIMITER;
  const char delim2[] = MSG_END;
  char *msg_ptr = NULL;
  char *msg_length_ptr = NULL;
  char** bufferPos = &buffer;

  msg_length_ptr = strtok_r(buffer, delim1, bufferPos);
  msg_ptr = strtok_r(NULL, delim2, bufferPos);
  msg_length = atoi(msg_length_ptr);
  if (msg_length != -1 && msg_length < size)
  {
    decodeNetstring(msg_ptr);
  }
}

/*
 * Processes the command. Setting PINs, PWMs.
 *
 * Param1: command to set.
 * Param2: value to set.
 * Return: 0 if successful, -1 otherwise.
 */
int processCommand(char* sensor, char* value)
{
  int result = 0;
  enum {
    NONE = 0,
    GPIO = 1,
    PWM = 2,
  } type;
  unsigned int pinID = 0;

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
  else
  {
    result = -1;
  }

  if (result == 0)
  {
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
            pwmEnableChannel(&PWMD4, 3, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, intValue)); break;
        }
        break;
      default:
        result = -1;
        break;
    }
  }

  return result;
}

/*
 * Process status request
 *
 * returns: length of status message to be transmitted
 */
size_t processStatus(void)
{
  size_t payloadLength = 0;
  //Analog input
  uint32_t ebs_line = (uint32_t)(samples[0] + samples[6] + samples[12] + samples[18]) / 4; //PC0
  uint32_t ebs_actuator = (uint32_t)(samples[1] + samples[7] + samples[13] + samples[19]) / 4; //PC1
  uint32_t pressure_rag = (uint32_t)(samples[2] + samples[8] + samples[14] + samples[20]) / 4; //PC2
  uint32_t service_tank = (uint32_t)(samples[3] + samples[9] + samples[15] + samples[21]) / 4; //PC3
  uint32_t position_rack = (uint32_t)(samples[4] + samples[10] + samples[16] + samples[22]) / 4; //PC4
  uint32_t steer_pos = (uint32_t)(samples[5] + samples[11] + samples[17] + samples[23]) / 4; //PC5
  //Digital input
  bool asms = palReadPad(GPIOC, 13); //PC13
  bool clamped_sensor = palReadPad(GPIOC, 14); //PC14
  bool ebs_ok = palReadPad(GPIOC, 15); //PC15

  payloadLength = sprintf(writeBuffer, "get|status|ACK"
                       "|ebs_line|%ld|ebs_actuator|%ld|pressure_rag|%ld"
                       "|service_tank|%ld|position_rack|%ld|steer_pos|%ld"
                       "|asms|%d|clamped_sensor|%d|ebs_ok|%d",
                       ebs_line, ebs_actuator, pressure_rag,
                       service_tank, position_rack, steer_pos,
                       asms, clamped_sensor, ebs_ok);

  return payloadLength;
}

/*
 * Send message through the USB
 *
 * Param1: message
 * Param2: message size
 */
void sendMessage(char* msg, size_t size)
{
  char temp[sizeof(writeBuffer)];
  size_t msgLength = sprintf(temp, "%d:%s;", size, msg);
  chnWrite(&PORTAB_SDU1, (uint8_t*)temp, msgLength);
}

/*
 * Decodes request and calls processing function depending on the command
 * Sends back the result of execution, either requested status or ack/nack.
 *
 * Parameter: message.
 */
void decodeNetstring(char* msg)
{
  int result = 0;
  const char delim[] = DELIMITER;
  char **temp = &msg;
  char* command = strtok_r(msg, delim, temp);
  char* sensor = strtok_r(NULL, delim, temp);
  char* value = strtok_r(NULL, delim, temp);
  size_t msgLength = 0;

  if (strstr(command, SET))
  {
    result = processCommand(sensor, value);
    msgLength = result == 0 ?
            sprintf(writeBuffer, "%s|%s|%s|%s", command, sensor, value, ACK) :
            sprintf(writeBuffer, "%s|%s|%s|%s", command, sensor, value, NACK);
  }
  else if (strstr(command, GET))
  {
    msgLength = processStatus();
  }
  else
  {
    result = -3;
  }

  /* send response */
  sendMessage(writeBuffer, msgLength);
}
