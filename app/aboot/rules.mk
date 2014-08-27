LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifeq ($(ENABLE_2NDSTAGE_BOOT),1)
GLOBAL_DEFINES += BOOT_2NDSTAGE=1
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/aboot.c \
	$(LOCAL_DIR)/recovery.c

include make/module.mk
