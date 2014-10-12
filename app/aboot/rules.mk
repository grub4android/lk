LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/aboot.c \
	$(LOCAL_DIR)/recovery.c

include make/module.mk
