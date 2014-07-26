LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/api.o \
	$(LOCAL_DIR)/api_storage.o \
	$(LOCAL_DIR)/uboot_part.o



