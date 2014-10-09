LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
    $(LOCAL_DIR)/gcdb_display.c \
    $(LOCAL_DIR)/panel_display.c \
    $(LOCAL_DIR)/gcdb_autopll.c

include make/module.mk
