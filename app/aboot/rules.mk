LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += ASSERT_ON_TAMPER=1

MODULE_SRCS += \
	$(LOCAL_DIR)/aboot.c \
	$(LOCAL_DIR)/fastboot.c \
	$(LOCAL_DIR)/recovery.c

include make/module.mk
