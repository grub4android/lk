LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/target.c \
	$(LOCAL_DIR)/acpuclock.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/clock.c \
	$(LOCAL_DIR)/keypad.c

KEYS_USE_GPIO_KEYPAD := 1

MEMBASE := 0x88f00000
MEMSIZE := 0x20000000	# 512MB
KERNEL_BASE := 0xc0000000

MODULE_DEPS += \
	platform/qcom \
	dev/pmic/pm8921 \
	dev/ssbi \
	dev/keys

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

GLOBAL_CFLAGS += -DQCOM_ADDITIONAL_INCLUDE="<platform/msm8960.h>"

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
