ifndef DEVKITPRO
$(error Set DEVKITPRO to the devkitPro installation path)
endif
CC := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-gcc
TOOLS := $(DEVKITPRO)/tools/bin
FLAGS := -std=gnu11 -O2 -Wall -Wextra -Werror -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -D__SWITCH__
FLAGS += -I$(DEVKITPRO)/libnx/include -specs=$(DEVKITPRO)/libnx/switch.specs
.PHONY: all clean
all: build/runtime.nso
build/runtime.elf: native/main.c
	mkdir -p build
	$(CC) $(FLAGS) $< -L$(DEVKITPRO)/libnx/lib -lnx -o $@
build/runtime.nso: build/runtime.elf
	$(TOOLS)/elf2nso $< $@
clean:
	rm -rf build
