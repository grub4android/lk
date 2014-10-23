LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/pf2font \
	app/display_server

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

# LKFONT
GLOBAL_CFLAGS += -DLKFONT_HEADER=\"$(LKFONT_HEADER)\"

MODULE_SRCS += \
	$(LOCAL_DIR)/render.c \
	$(LOCAL_DIR)/menu_main.c \
	$(LOCAL_DIR)/menu_settings.c

include make/module.mk
