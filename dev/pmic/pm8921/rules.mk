LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/pm8921.c \
	$(LOCAL_DIR)/pm8921_pwm.c \
	$(LOCAL_DIR)/pm8921_rtc.c

include make/module.mk

