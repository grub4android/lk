LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/fastboot.c

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

include make/module.mk
