LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/target_display.c

include make/module.mk

