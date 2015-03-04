LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := qcom-msm8610

MODULE_SRCS += \
	$(LOCAL_DIR)/target.c

include make/module.mk

