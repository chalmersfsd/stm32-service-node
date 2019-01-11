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
/* for how to use ADC, see: https://github.com/ashfaqfarooqui/CaroloCup/wiki/ADC-in-ChibiOS */
static void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   6
/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      4
/*
 * ADC samples buffer.
 */
static adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
static const ADCConversionGroup adcgrpcfg = {
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
/* PWM Read related stuff.                                                   */
/*===========================================================================*/
/*
 * PWM configuration
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
  /*
    palClearPad(GPIOD, GPIOD_LED3);
    chThdSleepMilliseconds(time);
    palSetPad(GPIOD, GPIOD_LED3);
    chThdSleepMilliseconds(time);
  */
  unsigned int pin = GPIOD_PIN1;
  palWritePad(GPIOD, GPIOD_LED3, PAL_HIGH);
  chThdSleepMilliseconds(time);
  palWritePad(GPIOD, GPIOD_LED3, PAL_LOW);
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
unsigned int decodedValue = 0;
void decodeRequest(uint8_t* msg){
		char* readptr = msg;
		char* pin = NULL;
		char* valuePtr = NULL;
		bool foundPin = false;
		bool isGPIO = false;
		bool isPWM = false;
		unsigned int pinNameLength = 0;
		unsigned int pinID = 0;
		// digital out request
		if(!foundPin){//PD0
			pin = strstr(readptr, HEART_BEAT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(HEART_BEAT);		
				pinID = 0;
			}
		}
		
		if(!foundPin){//PD1
			pin = strstr(readptr, RACK_RIGHT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(RACK_RIGHT);
				pinID = 1;			
			}
		}
		
		if(!foundPin){//PD2
			pin = strstr(readptr, RACK_LEFT);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(RACK_LEFT);
				pinID = 2;			
			}
		}
		
		if(!foundPin){//PD3
			pin = strstr(readptr, SERVICE_BREAK);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SERVICE_BREAK);
				pinID = 3;			
			}
		}
		
		if(!foundPin){//PD4
			pin = strstr(readptr, REDUNDENCY);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(REDUNDENCY);
				pinID = 4;			
			}
		}
		
		if(!foundPin){//PD6
			pin = strstr(readptr, SHUTDOWN);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SHUTDOWN);
				pinID = 6;			
			}
		}
		
		if(!foundPin){//PD7
			pin = strstr(readptr, SPARE);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(SPARE);
				pinID = 7;			
			}
		}
		
		if(!foundPin){//PD8
			pin = strstr(readptr, CLAMP_SET);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(CLAMP_SET);
				pinID = 8;			
			}
		}
		
		if(!foundPin){//PD9
			pin = strstr(readptr, COMPRESSOR);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(COMPRESSOR);
				pinID = 9;			
			}
		}
		
		if(!foundPin){//PD10
			pin = strstr(readptr, EBS_RELIEF);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(EBS_RELIEF);
				pinID = 10;			
			}
		}
		
		if(!foundPin){//PD11
			pin = strstr(readptr, EBS_SPEAKER);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(EBS_SPEAKER);
				pinID = 11;			
			}
		}
		
		if(!foundPin){//PD12
			pin = strstr(readptr, FINISHED);
			if(pin){
				foundPin = true;
				isGPIO = true;
				pinNameLength = strlen(FINISHED);
				pinID = 12;			
			}
		}
		
		// PWM request
		if(!foundPin){//PB5
			pin = strstr(readptr, STEER_SPEED);
			if(pin){
				foundPin = true;
				isPWM = true;
				pinNameLength = strlen(STEER_SPEED);
				pinID = 5;		
			}
		}
		
		if(!foundPin){//PB6
			pin = strstr(readptr, BRAKE_PRESSURE);
			if(pin){
				foundPin = true;
				isPWM = true;
				pinNameLength = strlen(BRAKE_PRESSURE);
				pinID = 6;			
			}
		}
		
		if(!foundPin){//PB7
			pin = strstr(readptr, ASSI_BLUE);
			if(pin){
				foundPin = true;
				isPWM = true;
				pinNameLength = strlen(ASSI_BLUE);
				pinID = 7;			
			}
		}
		
		if(!foundPin){//PB8
			pin = strstr(readptr, ASSI_RED);
			if(pin){
				foundPin = true;
				isPWM = true;
				pinNameLength = strlen(ASSI_RED);
				pinID = 8;			
			}
		}
		
		if(!foundPin){//PB9
			pin = strstr(readptr, ASSI_GREEN);
			if(pin){
				foundPin = true;
				isPWM = true;
				pinNameLength = strlen(ASSI_GREEN);
				pinID = 9;			
			}
		}
		
		if(pin && isGPIO){
			valuePtr = pin + pinNameLength + 1;
			if(*valuePtr-48 == 1)
				palWritePad(GPIOD, pinID, PAL_HIGH);
			else
				palWritePad(GPIOD, pinID, PAL_LOW);		
		}
		else
		if(pin && isPWM){
			valuePtr = pin + pinNameLength + 1;
			unsigned int dutyCycle = strtol(valuePtr, NULL, 10);
      decodedValue = dutyCycle;
	    
	    switch(pinID){
	      case 5: // linear actuator steer spped
	        pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, dutyCycle)); break;
	      case 6: // pressure regulator
	        pwmEnableChannel(&PWMD4, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, dutyCycle)); break;
	      case 7: // ASSI blue
	        pwmEnableChannel(&PWMD4, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, dutyCycle)); break;
	      case 8: // ASSI red
	        pwmEnableChannel(&PWMD4, 2, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, dutyCycle)); break;
	      case 9: // ASSI green
	        pwmEnableChannel(&PWMD4, 3, PWM_PERCENTAGE_TO_WIDTH(&PWMD4, dutyCycle)); break;
	    }
		}
}
//READ THREAD
//Constantly read until reach '\n', receive command from docker to turn on/off green led
//------------------------------SYNC THIS CODE WITH MAX--------------------------------------
void decodeNextNetstring(void) {
	// Netstrings have the following format:
	// ASCII Number representing the length of the payload + ':' + payload + MSG_END
    
	// Start decoding only if we have received enough data.
	if (writePtr > 3) {
		char *colonSign = NULL;
		
		
  		
		unsigned int lengthOfPayload = strtol(receiveBuffer, &colonSign, 10);
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
				memcpy(messageBuffer, colonSign + 1, lengthOfPayload + 1);
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


static WORKING_AREA(readThreadWA, 1024);
static msg_t readThread(void *arg) {
  chRegSetThreadName("read thread");
  while (TRUE) {
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
			// Try to decode netstrings from receiveBuffer.
			consumeNetstrings();
		}
  	// Allow for thread scheduling.
    chThdSleepMilliseconds(5);
  }  	
}

//WRITE THREAD
static WORKING_AREA(writeThreadArea, 1024);
static msg_t writeThread(void *arg) {
  (void)arg;
  chRegSetThreadName("write thread");
  while (TRUE) {  	
  		//Analog input
  		uint32_t raw0 = (uint32_t)(samples[0] + samples[6] + samples[12] + samples[18]) / 4; //PC0 
  		uint32_t raw1 = (uint32_t)(samples[1] + samples[7] + samples[13] + samples[19]) / 4; //PC1 	
  		uint32_t raw2 = (uint32_t)(samples[2] + samples[8] + samples[14] + samples[20]) / 4; //PC2
  		uint32_t raw3 = (uint32_t)(samples[3] + samples[9] + samples[15] + samples[21]) / 4; //PC3
  		uint32_t raw4 = (uint32_t)(samples[4] + samples[10] + samples[16] + samples[22]) / 4; //PC4
  		uint32_t raw5 = (uint32_t)(samples[5] + samples[11] + samples[17] + samples[23]) / 4; //PC5		
  		//Digital input
  		bool raw6 = palReadPad(GPIOC, 13); //PC13
      bool raw7 = palReadPad(GPIOC, 14); //PC14
      bool raw8 = palReadPad(GPIOC, 15); //PC15
  		//write buffer
  		char payloadBuffer[256];
  		char writeBuffer[256];
			
			/*
			int payloadLength = sprintf(payloadBuffer, "status|ebs_line|%d|ebs_actuator|%d|pressure_rag|%d|service_tank|%d|position_rack|%d|steer_pos|%d|asms|%d|clamped_sensor|%d|ebs_ok|%d", raw0, raw1, raw2, raw3, raw4, raw5, raw6, raw7, raw8);
			*/
			int payloadLength = sprintf(payloadBuffer, "status|ebs_line|%d", raw6);
			//added a whitespace at the beiginning to fix the unknwon bug
			int bytesToWrite = sprintf(writeBuffer, " %d:%s;", payloadLength,payloadBuffer);
  		int bytesToWriteLeft = bytesToWrite;
  		int bytesWritten = 0;
  		
  		while(bytesToWriteLeft > 0){
  			bytesWritten = sdWriteTimeout(&SDU1, (uint8_t*)writeBuffer, bytesToWrite, timeOut);
  			bytesToWriteLeft -= bytesWritten;
  		}

  		chThdSleepMilliseconds(10);
  	/*
  	 //Debug
		 bytesToWrite = strlen(receiveBuffer);
  		 bytesToWriteLeft = bytesToWrite;
  		 bytesWritten = 0;
  		
  		while(bytesToWriteLeft > 0){
  			bytesWritten = sdWriteTimeout(&SDU1, (uint8_t*)receiveBuffer, bytesToWrite, timeOut);
  			bytesToWriteLeft -= bytesWritten;
  		}
  	*/

  	chThdSleepMilliseconds(5);
  }
}

//SAMPLE ADC 
static WORKING_AREA(adcSampleThreadWA, 64);
static msg_t adcSampleThread(void *arg) {
  (void)arg;
  chRegSetThreadName("Sample ADC");
  while (TRUE) {
  	adcConvert(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
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
   * Initializes the ADC driver 1
   * The pin PC0-PC5 on the port GPIOC is programmed as analog input.
   */
  palSetGroupMode(GPIOC, PAL_PORT_BIT(0), 0, PAL_MODE_INPUT_ANALOG); //Pin PC0
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1), 0, PAL_MODE_INPUT_ANALOG); //Pin PC1
  palSetGroupMode(GPIOC, PAL_PORT_BIT(2), 0, PAL_MODE_INPUT_ANALOG); //Pin PC2
  palSetGroupMode(GPIOC, PAL_PORT_BIT(3), 0, PAL_MODE_INPUT_ANALOG); //Pin PC3
  palSetGroupMode(GPIOC, PAL_PORT_BIT(4), 0, PAL_MODE_INPUT_ANALOG); //Pin PC4
  palSetGroupMode(GPIOC, PAL_PORT_BIT(5), 0, PAL_MODE_INPUT_ANALOG); //Pin PC5
  adcStart(&ADCD1, NULL);

  /*
   * Initializes the PWM driver 3 & 4, then set to zero duty cycle
   */
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
  
  // Creates the blinker thread. 
  chThdCreateStatic(usbThreadWA, sizeof(usbThreadWA), LOWPRIO, usbThread, NULL);
  // READ thread
  chThdCreateStatic(readThreadWA, sizeof(readThreadWA), HIGHPRIO, readThread, NULL);
  // WRITE thread
  chThdCreateStatic(writeThreadArea, sizeof(writeThreadArea), NORMALPRIO, writeThread, NULL);
  // ADC read thread
  chThdCreateStatic(adcSampleThreadWA, sizeof(adcSampleThreadWA), LOWPRIO, adcSampleThread, NULL);
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (TRUE) {
    chThdSleepMilliseconds(10);

  }
}
