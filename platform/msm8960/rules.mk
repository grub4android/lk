LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a8
CPU     := generic

GLOBAL_DEFINES += ARM_CPU_CORE_KRAIT

MMC_SLOT         := 1

GLOBAL_DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT) MDP4=1 SSD_ENABLE

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

DEVS += fbcon
MODULE_DEPS += \
	dev/fbcon \
	platform/msm_shared

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/acpuclock.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/clock.c

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include make/module.mk
