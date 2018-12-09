# stm32-service-node
Replacement for BeagleBone to handle readings from car's non-driverless sensors.

`STM32CubeMX-app` application for setting up initial STM32 configuration
`STM32F407VGTx` stuff related to our microcontroller - documents and CubeMX project file

## Getting it working
Utilizing Makefile in root directory. Help output is available, free to run `make` and read it.
* Build all tools, firmware and flash: `make all`
* Create docker images for building and flashing firmware: `make builder && make flasher`
* Build firmware (given you have corresponding docker image): `make build`
* Flash firmware `make flash`

To build flash something specific - modify the Makefile variables
