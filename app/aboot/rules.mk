LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LK_TOP_DIR)/platform/msm_shared/include

DEFINES += GRUB_LOADING_ADDRESS=$(GRUB_LOADING_ADDRESS)

OBJS += \
	$(LOCAL_DIR)/aboot.o \
	$(LOCAL_DIR)/fastboot.o \
	$(LOCAL_DIR)/recovery.o \
	$(LOCAL_DIR)/grub.o

include $(LOCAL_DIR)/uboot_api/rules.mk
