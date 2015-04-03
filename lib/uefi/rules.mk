LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bio \
	lib/rtc \
	lib/sysparam \
	dev/fbcon \
	dev/keys

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/peloader.c \
	$(LOCAL_DIR)/api.c \
	$(LOCAL_DIR)/api_runtime.c \
	$(LOCAL_DIR)/api_console.c \
	$(LOCAL_DIR)/api_simpletext.c \
	$(LOCAL_DIR)/api_blockio.c \
	$(LOCAL_DIR)/api_gop.c

ifneq ($(GRUB_BOOT_PARTITION),)
MODULE_CFLAGS += -DGRUB_BOOT_PARTITION=\"$(GRUB_BOOT_PARTITION)\"
endif
MODULE_CFLAGS += -DGRUB_BOOT_PATH_PREFIX=\"$(GRUB_BOOT_PATH_PREFIX)\"

include make/module.mk
