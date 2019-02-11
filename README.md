# stm32-service-node

### How it works
- This is firmware for STM32F4 to communicate with the new microservices of CFSD19 over USB serial. This firmware is written in ChibiOS 2.6.5. The service code can be found at https://github.com/chalmersfsd/opendlv-device-stm32-lynx/tree/development
- STM32 receives GPIO/PWM requests from Docker side, and execute these requests. It also continuously sends the analog/digital measurements to Docker side via serial-over-usb connection. The send/receive functions are independent from each other.

### Dependencies
- ChibiOS 2.6.5. This is included in the repository.
- STM32F4 running a serial-over-usb connection, and is connected to host machine, and appears as /dev/ttyACM0.
- Dockerized microservice running OpenDLV. This service can be found at https://github.com/chalmersfsd/opendlv-device-stm32-lynx/tree/development.
- Pin configuration is described in "stm32_pin_map.xlsx", and is configured at ChibiOS_2.6.5/board/ST_STM32F4_DISCOVERY/board.h. Change this file if you want another pin config. A copy of this file can also be found in the folder "board".

### Getting it working
Utilizing Makefile in root directory. Help output is available, free to run `make` and read it.
* Build all tools, firmware and flash: `make all`
* Create docker images for building and flashing firmware: `make builder && make flasher`
* Build firmware (given you have corresponding docker image): `make build`
* Flash firmware `make flash`

### Function disceription
The code has 4 threads, whose purposes and related functions are described below:

- usbThread: blink the orange LED according to the Serial over USB status; slow blink (1Hz) means no connection, and fast blink (2Hz) means good connection.

- readThread: collects & decode the Netstring bytes sent from Docker side. The bytes are first received using sdReadTimeout & stored in chunkBuffer (a temporary buffer), then receiveBuffer (this is the main storing buffer, where most read actions are performed on). A char* pointer, writePtr, is used to keep track of where in receiveBuffer the reading is taking place. This is done by the following functions:
  - consumeNetstring(): a temporary function to remember the position of writePtr.
  - decodeNextNetstring(): the main reading function. It parses the bytes in receiveBuffer, looking for a valid Netstring by looking for the number of bytes (payload length of Netstring) using strtol. Once that number is found, it checks for the end delimeter of the Netstring (';'); if that is also found, then it means it found a valid Netstring, and will extract & decode the payload in decodeRequest(). The extracted payload is stored in messageBuffer. After that, the read Netstring are removed from receiveBuffer by memmove, and by moving the writePtr.
  - decodeRequest(): decodes the payload based on a pre-defined mesage format. The format is: "set|pinID|value". E.g: "set|compressor|1", "set|ebs_line|0", "set|steer_speed|50000",...

- writeThread: sends measurements to Docker side. The procedure is the opposite of reading: the measurements are first encoded according to the pre-defined format,e.g: "status|ebs_line|1234", "status|ebs_actuator|1234", then encoded as Netstring, and finally sent over USB using sdWriteTimeout().

- adcSampleThread: samples the raw Analog measurements.

### Known issue
1) An unknown bug caused the first byte in the sent bytes to appear at the beginning of the received bytes. This only happens when both read & write threads are executed. This prevents the reading thread to succesfully extract the netstring messages (since strtol() only works for strings with leading white spaces). A temporary fix is to add an additional white space character at the begining of the sent bytes. This fix has worked so far with all of the signals included.

2) Originally the measurements were sent in a single message, i.e: "status|ebs_line|1234|ebs_actuator|1234", but sometimes this cause the stm to not be able to receive & decode requests. A fix of this is to split the original message into several messages, each containing one measurement.


### To-do
- Re-calibrate sensor measurements.
- Analize respons time, especially to critical signals (heart_beat).
- Re-organize code files. Currently most codes are written in one single main.c file.
- Dockerize the build-flash procedure
- Adjust indentations for better code.
- Continue debugging to find possible bugs
