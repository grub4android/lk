LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
QCOM_ENABLE_EMMC := 1
QCOM_ENABLE_DISPLAY := 1
QCOM_DISPLAY_TYPE_MDP4 := 1
QCOM_DISPLAY_TYPE_MIPI := 1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/target.c \
	$(LOCAL_DIR)/acpuclock.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/clock.c \
	$(LOCAL_DIR)/keypad.c \
	$(LOCAL_DIR)/mmap.c

KEYS_USE_GPIO_KEYPAD := 1

MEMBASE := 0x80000000
MEMSIZE := 0x02000000	# 32MB
KERNEL_LOAD_OFFSET := 0x08f00000
LINUX_BASE := 0x80200000
LINUX_SIZE := 0x2c00000 # 44MB

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
