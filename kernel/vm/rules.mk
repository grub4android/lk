LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/bootalloc.c \
	$(LOCAL_DIR)/pmm.c \
	$(LOCAL_DIR)/vm.c

include make/module.mk
