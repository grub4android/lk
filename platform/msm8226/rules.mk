LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH    := arm
#Compiling this as cortex-a8 until the compiler supports cortex-a7
ARM_CPU := cortex-a8
CPU     := generic

GLOBAL_DEFINES += ARM_CPU_CORE_A7

MMC_SLOT         := 1

GLOBAL_DEFINES += PERIPH_BLK_BLSP=1
GLOBAL_DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT) SSD_ENABLE

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

DEVS += fbcon
MODULE_DEPS += \
	dev/fbcon \
	platform/msm_shared

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/acpuclock.c \
	$(LOCAL_DIR)/msm8226-clock.c \
	$(LOCAL_DIR)/gpio.c

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include make/module.mk
