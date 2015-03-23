LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bio \
	lib/rtc \
	lib/sysparam

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/peloader.c \
	$(LOCAL_DIR)/api.c \
	$(LOCAL_DIR)/api_runtime.c \
	$(LOCAL_DIR)/api_console.c \
	$(LOCAL_DIR)/api_simpletext.c \
	$(LOCAL_DIR)/api_blockio.c \
	$(LOCAL_DIR)/api_gop.c

include make/module.mk
