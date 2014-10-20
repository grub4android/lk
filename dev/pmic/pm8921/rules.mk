LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/pm8921.c \
	$(LOCAL_DIR)/pm8921_pwm.c

include make/module.mk

