LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += WITH_MMU_CALLBACK=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/clock.c \
	$(LOCAL_DIR)/clock_pll.c \
	$(LOCAL_DIR)/board.c \
	$(LOCAL_DIR)/smem.c \
	$(LOCAL_DIR)/smem_ptable.c \
	$(LOCAL_DIR)/qcom_ptable.c \
	$(LOCAL_DIR)/hsusb.c \
	$(LOCAL_DIR)/scm.c

MODULE_DEPS += \
	dev/interrupt/arm_gic

ifeq ($(QCOM_ENABLE_UART),1)
GLOBAL_DEFINES += QCOM_ENABLE_UART
MODULE_SRCS += \
	$(LOCAL_DIR)/uart_dm.c
endif

ifeq ($(QCOM_ENABLE_EMMC),1)
GLOBAL_DEFINES += QCOM_ENABLE_EMMC

ifeq ($(QCOM_ENABLE_SDHCI),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/mmc_sdhci.c \
	$(LOCAL_DIR)/gpio.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/mmc.c
endif
endif

ifeq ($(QCOM_ENABLE_SDHCI),1)
GLOBAL_DEFINES += QCOM_ENABLE_SDHCI
MODULE_SRCS += \
	$(LOCAL_DIR)/sdhci.c \
	$(LOCAL_DIR)/sdhci_msm.c
endif

ifeq ($(QCOM_ENABLE_QTIMER),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/qtimer.c \
	$(LOCAL_DIR)/qtimer_mmap.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/timer.c
endif

ifeq ($(QCOM_ENABLE_CLOCK_LIB2),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/clock_lib2.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/clock-local.c
endif

ifeq ($(QCOM_ENABLE_SPMI),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/spmi.c
endif

ifeq ($(QCOM_ENABLE_SMD_SUPPORT),1)
GLOBAL_DEFINES += QCOM_ENABLE_SMD_SUPPORT
MODULE_SRCS += \
	$(LOCAL_DIR)/smd.c \
	$(LOCAL_DIR)/rpm-smd.c
endif

ifeq ($(QCOM_MMU_IDENTITY_MAP),1)
GLOBAL_DEFINES += QCOM_MMU_IDENTITY_MAP
endif

include make/module.mk
