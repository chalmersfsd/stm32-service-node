SOURCE_DIR=$(PWD)
MAKEFILE_DIR=ProjectFormula
help: ## === Tools for building and deploying firmware to STM32F407Discovery board ===
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | sed -e 's/##//'

.PHONY: builder
builder: ## Build the stm_builder:latest image for software complation
	mkdir -p dummy_context
	docker build -f BuilderDockerfile.amd64 -t stm_builder:latest dummy_context
	rmdir dummy_context

.PHONY: flasher
flasher: ## Build the stm_flasher:latest image for binary flashing
	mkdir -p dummy_context
	docker build -f FlasherDockerfile.amd64 -t stm_flasher:latest dummy_context
	rmdir dummy_context

.PHONY: build
build: ## Run the compilation by means of image created by `make builder`
	docker run --rm -v $(SOURCE_DIR):/sources stm_builder:latest $(MAKEFILE_DIR)

.PHONY: flash
flash: ## Flash the firmware by means of image created by `make flasher`
	docker run --rm --privileged -v /dev/bus/usb:/dev/bus/usb -v $(SOURCE_DIR)/$(MAKEFILE_DIR)/build:/binaries stm_flasher:latest

.PHONY: clean
clean: ## Remove the build
	docker run --rm -v $(SOURCE_DIR):/sources stm_builder:latest $(MAKEFILE_DIR) clean

## all: Build images, build firmware, flash firmware
all: builder flasher build flash
