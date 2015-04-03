LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifneq ($(WITH_LIB_ZLIB),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/adler32.c \
	$(LOCAL_DIR)/crc32.c
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/crc16.c \
	$(LOCAL_DIR)/debug.c

MODULE_CFLAGS += -Wno-strict-prototypes

include make/module.mk
