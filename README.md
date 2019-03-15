# stm32-service-node

### How it works
- This is firmware for STM32F4 to communicate with the new microservices of CFSD over USB serial. The service code can be found at https://github.com/chalmersfsd/opendlv-device-stm32-lynx/tree/development
- STM32 receives GPIO/PWM requests from Docker side, and execute these requests.
- It it request-response communication where Docker side should send requests and STM would respond to them.

### Dependencies
- ChibiOS 18.2. This is included in the repository.
- STM32F4 running a serial-over-usb connection (micro-usb), drivers for the board installed.
- Dockerized microservice running OpenDLV. This service can be found at https://github.com/chalmersfsd/opendlv-device-stm32-lynx/tree/development.
- Pin configuration is described in "stm32_pin_map.xlsx", and is configured at src/chibios182/os/hal/boards/ST_STM32F4_DISCOVERY/board.h. Change this file if you want another pin config.

### Getting it working
Utilizing Makefile in root directory. Help output is available, free to run `make` and read it.
* Build all tools, firmware and flash: `make all`
* Create docker images for building and flashing firmware: `make builder && make flasher`
* Build firmware (given you have corresponding docker image): `make build`
* Flash firmware `make flash`

### Function description
The code has 3 threads, whose purposes and related functions are described below:

- usbThread: blink the orange LED according to the Serial over USB status; slow blink (1Hz) means no connection, and fast blink (2Hz) means good connection.

- readThread: collects & decode the Netstring bytes sent from Docker side. The bytes are first received using sdReadTimeout & stored in chunkBuffer (a temporary buffer), then receiveBuffer (this is the main storing buffer, where most read actions are performed on). A char* pointer, writePtr, is used to keep track of where in receiveBuffer the reading is taking place. This is done by the following functions:
  - consumeNetstring(): a temporary function to remember the position of writePtr.
  - decodeNextNetstring(): the main reading function. It parses the bytes in receiveBuffer, looking for a valid Netstring by looking for the number of bytes (payload length of Netstring) using strtol. Once that number is found, it checks for the end delimeter of the Netstring (';'); if that is also found, then it means it found a valid Netstring, and will extract & decode the payload in decodeRequest(). The extracted payload is stored in messageBuffer. After that, the read Netstring are removed from receiveBuffer by memmove, and by moving the writePtr.
  - decodeRequest(): decodes the payload based on a pre-defined mesage format. The format is: "set|pinID|value". E.g: "set|compressor|1", "set|ebs_line|0", "set|steer_speed|50000",...

- writeThread: sends measurements to Docker side. The procedure is the opposite of reading: the measurements are first encoded according to the pre-defined format,e.g: "status|ebs_line|1234", "status|ebs_actuator|1234", then encoded as Netstring, and finally sent over USB using sdWriteTimeout().

- adcSampleThread: samples the raw Analog measurements.


### To-do
- Re-calibrate sensor measurements.
- Continue debugging to find possible bugs
