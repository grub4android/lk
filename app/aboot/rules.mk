LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/bio \
	dev/keys \
	lib/android

MODULE_SRCS += \
	$(LOCAL_DIR)/aboot.c

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

include make/module.mk
