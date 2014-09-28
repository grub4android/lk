# top level project rules for the msm8960_aries project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := msm8960_aries

MODULES += app/aboot

DEBUG := 1
EMMC_BOOT := 1

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_UART=1
DEFINES += WITH_DEBUG_LOG_BUF=1
#DEFINES += WITH_DEBUG_FBCON=1
DEFINES += WITH_XIAOMI_DUALBOOT=1
SPLASH_PARTITION_NAME := \"logo\"

#Disable thumb mode
#TODO: The gold linker has issues generating correct
#thumb interworking code for LK. Confirm that the issue
#is with the linker and file a bug rep
ENABLE_THUMB := false

ifeq ($(EMMC_BOOT),1)
DEFINES += _EMMC_BOOT=1
endif
