/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

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
#include "DiscoveryBoard.h"

/*===========================================================================*/
/* USB Read related stuff.                                                   */
/*===========================================================================*/
#define BUFFER_LENGTH 128
#define CHUNK_LENGTH 64
int writePtr = 0;
size_t bytesRead = 0;
char receiveBuffer[BUFFER_LENGTH];
char chunkBuffer[CHUNK_LENGTH];

/*===========================================================================*/
/* ADC related stuff.                                                        */
/*===========================================================================*/
static void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   2
/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      4
/*
 * ADC samples buffer.
 */
static adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
/*
 * ADC conversion group.
 * Mode:        Linear buffer, 4 samples of 2 channels, SW triggered.
 * Channels:    IN11   (48 cycles sample time)
 *              Sensor (192 cycles sample time)
 */
static const ADCConversionGroup adcgrpcfg = {
  FALSE, //FALSE is linear, TRUE is circular
  ADC_GRP1_NUM_CHANNELS,
  adccb, //callback
  NULL,
  /* HW dependent part.*/
  0, //CR1
  ADC_CR2_SWSTART, //CR2
  ADC_SMPR1_SMP_AN11(ADC_SAMPLE_56) | ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_56),
  0,
  ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),
  0,
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN11) | ADC_SQR3_SQ1_N(ADC_CHANNEL_SENSOR)
};

/*
 * ADC end conversion callback.
 * The PWM channels are reprogrammed using the latest ADC samples.
 * The latest samples are transmitted into a single SPI transaction.
 */
void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void) buffer; (void) n;
  /* Note, only in the ADC_COMPLETE state because the ADC driver fires an
     intermediate callback when the buffer is half full.*/
  if (adcp->state == ADC_COMPLETE) {
    adcsample_t avg_ch1, avg_ch2;
    /* Calculates the average values from the ADC samples.*/
    avg_ch1 = (samples[0] + samples[2] + samples[4] + samples[6]) / 4;
    avg_ch2 = (samples[1] + samples[3] + samples[5] + samples[7]) / 4;
  }
}

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/
systime_t timeOut = MS2ST(10); // Receiving and sending timeout is 10ms.
// Custom function: read until reach \n
size_t sdGetLine(SerialDriver *sdp, uint8_t *buf) {
  size_t n;
  uint8_t c;

  n = 0;
  do {
    c = sdGet(sdp);
    *buf++ = c;
    n++;
  } while (c != '\n');
  *buf = 0;
  return n;
}

// Custom function: read until reach \n
size_t sdPutLine(SerialDriver *sdp, uint8_t *buf) {
  size_t n;
  uint8_t c = buf;

  n = 0;
  do {
    sdWrite(sdp, c, 1);
    c++;
    n++;
  } while (c != '\n');
  *buf = 0;
  return n;
}

//Red LED blinker thread, times are in milliseconds.
static WORKING_AREA(usbThreadWA, 128);
static msg_t usbThread(void *arg) {
  (void)arg;
  chRegSetThreadName("usb");
  while (TRUE) {
    systime_t time;
		if(isUSBActive())
    	time=125;
    else
    	time=500;
    palClearPad(GPIOD, GPIOD_LED3);
    chThdSleepMilliseconds(time);
    palSetPad(GPIOD, GPIOD_LED3);
    chThdSleepMilliseconds(time);
  }
}
// Decode requests sent from docker
/* Requests are formatted as netstring: <number of bytes><':'><message><','>
both sender and receive must agree on number of bytes,
a new netstring starts after the delimiter ','
 */	
// A simple atoi() function 
int myAtoi(char *str) 
{ 
    int res = 0; // Initialize result 
   
    // Iterate through all characters of input string and 
    // update result 
    int i;
    for (i = 0; str[i] != '\0'; ++i) 
        res = res*10 + str[i] - '0'; 
   
    // return result. 
    return res; 
} 

void cats(char **str, const char *str2) {
    char *tmp = NULL;

    // Reset *str
    if ( *str != NULL && str2 == NULL ) {
        free(*str);
        *str = NULL;
        return;
    }

    // Initial copy
    if (*str == NULL) {
        *str = calloc( strlen(str2)+1, sizeof(char) );
        memcpy( *str, str2, strlen(str2) );
    }
    else { // Append
        tmp = calloc( strlen(*str)+1, sizeof(char) );
        memcpy( tmp, *str, strlen(*str) );
        *str = calloc( strlen(*str)+strlen(str2)+1, sizeof(char) );
        memcpy( *str, tmp, strlen(tmp) );
        memcpy( *str + strlen(*str), str2, strlen(str2) );
        free(tmp);
    }

}

char* messageBuffer[64];
void decodeRequest(uint8_t* msg){
		char* readptr = msg;
		char* pin = NULL;
		char* valuePtr = NULL;
		bool foundPin = false;
		bool isGPIO = false;
		unsigned int pinNameLength = 0;

		if(!foundPin){//1
			pin = strstr(readptr, HEART_BEAT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(HEART_BEAT);			
			}
		}
		
		if(!foundPin){//2
			pin = strstr(readptr, RACK_RIGHT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(RACK_RIGHT);			
			}
		}
		
		if(!foundPin){//3
			pin = strstr(readptr, RACK_LEFT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(RACK_LEFT);			
			}
		}
		
		if(!foundPin){//4
			pin = strstr(readptr, SERVICE_BREAK);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SERVICE_BREAK);			
			}
		}
		
		if(!foundPin){//5
			pin = strstr(readptr, REDUNDENCY);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(REDUNDENCY);			
			}
		}
		
		if(!foundPin){//6
			pin = strstr(readptr, SHUTDOWN);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SHUTDOWN);			
			}
		}
		
		if(!foundPin){//7
			pin = strstr(readptr, SPARE);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SPARE);			
			}
		}
		
		if(!foundPin){//8
			pin = strstr(readptr, CLAMP_SET);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(CLAMP_SET);			
			}
		}
		
		if(!foundPin){//9
			pin = strstr(readptr, COMPRESSOR);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(COMPRESSOR);			
			}
		}
		
		if(!foundPin){//10
			pin = strstr(readptr, EBS_RELIEF);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(EBS_RELIEF);			
			}
		}
		
		if(!foundPin){//11
			pin = strstr(readptr, EBS_SPEAKER);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(EBS_SPEAKER);			
			}
		}
		
		if(!foundPin){//12
			pin = strstr(readptr, FINISHED);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(FINISHED);			
			}
		}
		
		if(pin && isGPIO){
			valuePtr = pin + pinNameLength + 1;
			if(*valuePtr-48 == 1)
				palSetPad(GPIOD, GPIOD_LED5);
			else
				palClearPad(GPIOD, GPIOD_LED5);			
		}
}

char debugBuffer[128];
//READ THREAD
//Constantly read until reach '\n', receive command from docker to turn on/off green led
//------------------------------SYNC THIS CODE WITH MAX--------------------------------------
void decodeNextNetstring(void) {
	// Netstrings have the following format:
	// ASCII Number representing the length of the payload + ':' + payload + MSG_END
    
	// Start decoding only if we have received enough data.
	if (writePtr > 3) {
		char *colonSign = NULL;
		
		//Debug only

			//debugBuffer = colonSign;
			//int bytesToWrite = sprintf(debugBuffer, "lengthOfPayload = %d", lengthOfPayload);
			/*
			int bytesToWrite = strlen(receiveBuffer);
  		int bytesToWriteLeft = bytesToWrite;
  		int bytesWritten = 0;
  		if(receiveBuffer[0] == 'w')
  											palSetPad(GPIOD, GPIOD_LED4);
  		else
  										palClearPad(GPIOD, GPIOD_LED4);
  		while(bytesToWriteLeft > 0){
  			bytesWritten = sdWriteTimeout(&SDU1, (uint8_t*)receiveBuffer, bytesToWrite, timeOut);
  			bytesToWriteLeft -= bytesWritten;
  		}
  		*/
		uint32_t lengthOfPayload = strtol(receiveBuffer, &colonSign, 10);

		//if(lengthOfPayload == 16)
		if (*colonSign == 0x3a) {
			// Found colon sign.
 		
			// First, check if the buffer is as long as it is stated in the netstring.
			if (writePtr < (int)lengthOfPayload) {
			// Received data is too short. Skip further processing this part.

				return;
			}

			// Now, check if (receiveBuffer + 1 + lengthOfPayload) == MSG_END.
			if ((colonSign[1 + lengthOfPayload]) == ';') {
				// Successfully found a complete Netstring.
				memcpy(messageBuffer, colonSign + 1, lengthOfPayload);
				decodeRequest(messageBuffer);
				// Determine the size of Netstring.
				int lengthOfNetstring = (colonSign + 1 + lengthOfPayload + 1) - receiveBuffer;
				 // Remove decoded Netstring from receiveBuffer.
				memmove(receiveBuffer, colonSign + 1 + lengthOfPayload + 1, (BUFFER_LENGTH - lengthOfNetstring));

				// Move the writer pointer to the right position after consuming the read bytes.
				writePtr -= lengthOfNetstring;

				// Process successfully decoded payload.
				//processPayload();
			}
		}
	}    
}

void consumeNetstrings(void) {
  int oldWritePtr = 0; // Store the old position of the writePtr to make sure to not end up in an infinite loop.
  while ((writePtr > 3) && (oldWritePtr != writePtr)) {
  	oldWritePtr = writePtr;
  	decodeNextNetstring();
  }    
}

int what = 0;
void read(void ) {
  	
  	//Lock mutex
    bytesRead = sdReadTimeout(&SDU1, (uint8_t*)chunkBuffer, CHUNK_LENGTH, timeOut);
    //Releash mutex
    if (bytesRead > 0) {
			// Add received bytes to buffer to parse data from.
			//check if receiveBuffer has enough space left
			if(writePtr + bytesRead < BUFFER_LENGTH){
			memcpy(receiveBuffer+writePtr, chunkBuffer, bytesRead);
			writePtr += bytesRead;
			}
			
				if(what < 3 || receiveBuffer[0] == ';'){
			//read first time to get rid of random charactor
  		memmove(receiveBuffer, receiveBuffer + 1, strlen(receiveBuffer)-1);
  		what++;
  	}
  	
			// Try to decode netstrings from receiveBuffer.
			consumeNetstrings();
		}
  	// Allow for thread scheduling.
    chThdSleepMilliseconds(5);
	
}

static char writeBuffer[256];
void write(void) {

  		//ANALOG DATA
  		uint32_t raw = (uint32_t)(samples[1] + samples[3] + samples[5] + samples[7]) / 4; //PC1
  		char rawString[4]; //ADC value rang 0-4096 = 4 bytes of char
  		itoa(raw, rawString, 10);
  		
  		//write buffer
  		char payloadBuffer[256];

			
			int payloadLength = sprintf(payloadBuffer, "status|ebs_line|%d", raw);
			int bytesToWrite = sprintf(writeBuffer, "%d:%s;", payloadLength,payloadBuffer);
			//For debug
			//writeBuffer[0] = 'w';
  		int bytesToWriteLeft = bytesToWrite;
  		int bytesWritten = 0;
  		
  		while(bytesToWriteLeft > 0){
  			bytesWritten = sdWriteTimeout(&SDU1, writeBuffer, bytesToWrite, timeOut);
  			bytesToWriteLeft -= bytesWritten;
  		}

  		bytesToWrite = 0;
  	chThdSleepMilliseconds(5);

}

//SAMPLE ADC 
static WORKING_AREA(adcSampleThreadWA, 64);
static msg_t adcSampleThread(void *arg) {
  (void)arg;
  chRegSetThreadName("Sample ADC");
  while (TRUE) {
    //chSysLockFromIsr();
  	adcConvert(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
  	//chSysUnlockFromIsr();
  	//adcStopConversion(&ADCD1);
  	//chSysLockFromIsr();

  	//chSysUnlockFromIsr();
  }
}
/*===========================================================================*/
/* Application entry point.                                                             */
/*===========================================================================*/
int main(void) {
  Thread *shelltp = NULL;

  halInit();
  chSysInit();
  // Initialize interface to exchange data.
  initializeUSB(); // USB driver and USB-CDC link.
  /*
   * Initializes the ADC driver 1 and enable the thermal sensor.
   * The pin PC1 on the port GPIOC is programmed as analog input.
   */
  adcStart(&ADCD1, NULL);
  adcSTM32EnableTSVREFE();
  palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_ANALOG); // Positionrack (Nam)
  
  //chSysLockFromIsr();
  	//adcStartConversionI(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
  	//chSysUnlockFromIsr();
  

  // Creates the blinker thread. 
  chThdCreateStatic(usbThreadWA, sizeof(usbThreadWA), LOWPRIO, usbThread, NULL);
  
  // ADC read thread
  chThdCreateStatic(adcSampleThreadWA, sizeof(adcSampleThreadWA), LOWPRIO, adcSampleThread, NULL);
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (TRUE) {
    
    read();
      	chThdSleepMilliseconds(1);
    write();
      	chThdSleepMilliseconds(1);

  }
}
