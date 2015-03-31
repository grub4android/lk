LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := lib/android

MODULE_SRCS += \
	$(LOCAL_DIR)/fastboot.c \
	$(LOCAL_DIR)/commands.c

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

include make/module.mk
