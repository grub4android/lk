LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

QCOM_ENABLE_I2C_QUP := 1
QCOM_ENABLE_GSBI_I2C := 1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := qcom-msm8960

MODULE_SRCS += \
	$(LOCAL_DIR)/target.c \
	$(LOCAL_DIR)/target_display.c \
	$(LOCAL_DIR)/oem_panel.c

include make/module.mk

