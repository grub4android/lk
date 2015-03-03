LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += WITH_MMU_CALLBACK=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/clock.c \
	$(LOCAL_DIR)/clock-local.c \
	$(LOCAL_DIR)/clock_pll.c \
	$(LOCAL_DIR)/board.c \
	$(LOCAL_DIR)/smem.c \
	$(LOCAL_DIR)/smem_ptable.c \
	$(LOCAL_DIR)/qcom_ptable.c \
	$(LOCAL_DIR)/hsusb.c

MODULE_DEPS += \
	dev/interrupt/arm_gic

ifeq ($(QCOM_ENABLE_UART),1)
GLOBAL_DEFINES += QCOM_ENABLE_UART
MODULE_SRCS += \
	$(LOCAL_DIR)/uart_dm.c
endif

ifeq ($(QCOM_ENABLE_EMMC),1)
GLOBAL_DEFINES += QCOM_ENABLE_EMMC
MODULE_SRCS += \
	$(LOCAL_DIR)/mmc.c
endif

ifeq ($(QCOM_MMU_IDENTITY_MAP),1)
GLOBAL_DEFINES += QCOM_MMU_IDENTITY_MAP
endif

include make/module.mk
