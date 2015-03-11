LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	PLATFORM_HAS_DYNAMIC_TIMER=0

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_qcom_timer.c

include make/module.mk
