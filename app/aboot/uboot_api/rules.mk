LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bio

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/api.c \
	$(LOCAL_DIR)/api_storage.c \
	$(LOCAL_DIR)/api_display.c


include make/module.mk
