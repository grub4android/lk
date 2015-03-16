LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := qcom-msm8960

MODULE_SRCS += \
	$(LOCAL_DIR)/target.c

ifeq ($(QCOM_ENABLE_DISPLAY),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/target_display.c \
	$(LOCAL_DIR)/oem_panel.c
endif

include make/module.mk

