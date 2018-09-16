####
#### Makefile for building Aalto NEPPI 
####
#### The file system layout is:
#### ./Makefile
#### ./main.c
#### ../boards/neppi2018
#### ../RIOT
####

# Set the name of your application:
APPLICATION = AaltoNEPPI2018

# If no BOARD is found in the environment, use this default:
BOARD ?= neppi2018

# If using our custom board, need to setup this as well
ifneq (,$(filter neppi2018,$(BOARD)))
RIOTBOARD = $(CURDIR)/../boards
endif

# Uncomment this to disable optimizations for easier debugging:
CFLAGS_OPT ?= -O0

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

CFLAGS += -DNRF_SDH_BLE_VS_UUID_COUNT=1
CFLAGS += -DAPA102_PARAM_LED_NUMOF=4
CFLAGS += -DAPA102_PARAM_DATA_PIN=GPIO_PIN\(0,13\)
CFLAGS += -DAPA102_PARAM_CLK_PIN=GPIO_PIN\(0,15\)

# Configuration for running without LF crystal
# See http://infocenter.nordicsemi.com/index.jsp?\
# topic=%2Fcom.nordic.infocenter.s132.api.v6.0.0%2Fstructnrf__clock__lf__cfg__t.html&cp=2_3_1_1_0_2_8_4_0
CFLAGS += -DNRF_SDH_CLOCK_LF_SRC=0
CFLAGS += -DNRF_SDH_CLOCK_LF_RC_CTIV=6
CFLAGS += -DNRF_SDH_CLOCK_LF_RC_TEMP_CTIV=2
CFLAGS += -DNRF_SDH_CLOCK_LF_ACCURACY=1

# XXX
CFLAGS += -DAPA102_SHIFTED_COLORS=1

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP = 1

# Change this to 0 to show compiler invocation lines by default:
# QUIET ?= 0

# Modules to include

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
#USEMODULE += posix
USEMODULE += xtimer
USEMODULE += periph_uart
USEMODULE += apa102
USEMODULE += color
USEMODULE += mpu9250
USEMODULE += periph_adc
USEMODULE += cap_touch

# Packages to include

USEPKG += nordic_softdevice_ble

# Specify custom dependencies for your application here ...
# APPDEPS = app_data.h config.h

include $(RIOTBASE)/Makefile.include

# ... and define them here (after including Makefile.include,
# otherwise you modify the standard target):
#proj_data.h: script.py data.tar.gz
#	./script.py
