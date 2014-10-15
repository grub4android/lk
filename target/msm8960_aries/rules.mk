LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := msm8960

BASE_ADDR        := 0x80200000

TAGS_ADDR        := BASE_ADDR+0x00000100
KERNEL_ADDR      := BASE_ADDR+0x00008000
RAMDISK_ADDR     := BASE_ADDR+0x01000000
SCRATCH_ADDR     := 0x90000000

KEYS_USE_GPIO_KEYPAD := 1

GLOBAL_DEFINES += DISPLAY_SPLASH_SCREEN=1
GLOBAL_DEFINES += DISPLAY_TYPE_MIPI=1
GLOBAL_DEFINES += DISPLAY_TYPE_HDMI=1

MODULE_DEPS += \
	dev/keys \
	dev/pmic/pm8921 \
	dev/ssbi \
	lib/ptable_msm \
	dev/gcdb/display \
	target/msm_shared

GLOBAL_DEFINES += \
	BASE_ADDR=$(BASE_ADDR) \
	TAGS_ADDR=$(TAGS_ADDR) \
	KERNEL_ADDR=$(KERNEL_ADDR) \
	RAMDISK_ADDR=$(RAMDISK_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR)

ifeq ($(LINUX_MACHTYPE_RUMI3), 1)
GLOBAL_DEFINES += LINUX_MACHTYPE_RUMI3
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/../msm8960/atags.c \
	$(LOCAL_DIR)/keypad.c \
	$(LOCAL_DIR)/target_display.c \
	$(LOCAL_DIR)/oem_panel.c

include make/module.mk

include target/$(TARGET)/tools/makefile
EXTRA_BUILDDEPS += APPSBOOTHEADER
