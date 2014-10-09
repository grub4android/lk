# top level project rules for the msm8960_virtio project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := msm8960

MODULES += \
	app/tests \
	app/stringtests \
	app/shell \
	app/fastboot

ifeq ($(TARGET_BUILD_VARIANT),user)
DEBUG := 0
else
DEBUG := 1
endif

#GLOBAL_DEFINES += WITH_DEBUG_DCC=1
GLOBAL_DEFINES += WITH_DEBUG_UART=1
#GLOBAL_DEFINES += WITH_DEBUG_FBCON=1

EMMC_BOOT := 1
ifeq ($(EMMC_BOOT),1)
GLOBAL_DEFINES += _EMMC_BOOT=1
endif
