LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
    LINUX_BASE=$(LINUX_BASE) \
    LINUX_SIZE=$(LINUX_SIZE)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
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


ifeq ($(QCOM_ENABLE_QGIC),1)
GLOBAL_DEFINES += QCOM_ENABLE_QGIC
MODULE_SRCS += \
	$(LOCAL_DIR)/qgic.c \
	$(LOCAL_DIR)/qgic_common.c \
	$(LOCAL_DIR)/interrupts.c
else
MODULE_DEPS += \
	dev/interrupt/arm_gic
endif

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
GLOBAL_DEFINES += QCOM_ENABLE_QTIMER
MODULE_SRCS += \
	$(LOCAL_DIR)/qtimer.c \
	$(LOCAL_DIR)/qtimer_mmap.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/timer.c
endif

ifeq ($(QCOM_ENABLE_CLOCK_LIB2),1)
GLOBAL_DEFINES += QCOM_ENABLE_CLOCK_LIB2
MODULE_SRCS += \
	$(LOCAL_DIR)/clock_lib2.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/clock-local.c
endif

ifeq ($(QCOM_ENABLE_SPMI),1)
GLOBAL_DEFINES += QCOM_ENABLE_SPMI
MODULE_SRCS += \
	$(LOCAL_DIR)/spmi.c
endif

ifeq ($(QCOM_ENABLE_I2C_QUP),1)
GLOBAL_DEFINES += QCOM_ENABLE_I2C_QUP
MODULE_SRCS += \
	$(LOCAL_DIR)/i2c_qup.c

ifeq ($(QCOM_ENABLE_GSBI_I2C),1)
GLOBAL_DEFINES += QCOM_ENABLE_GSBI_I2C
endif
endif

ifeq ($(QCOM_ENABLE_SMD_SUPPORT),1)
GLOBAL_DEFINES += QCOM_ENABLE_SMD_SUPPORT
MODULE_SRCS += \
	$(LOCAL_DIR)/smd.c \
	$(LOCAL_DIR)/rpm-smd.c
endif

ifeq ($(QCOM_ENABLE_DISPLAY),1)
MODULE_DEPS += $(LOCAL_DIR)/display
endif

ifeq ($(QCOM_MMU_IDENTITY_MAP),1)
GLOBAL_DEFINES += QCOM_MMU_IDENTITY_MAP
endif

ifeq ($(QCOM_MMU_SMEM_IMEM),1)
GLOBAL_DEFINES += QCOM_MMU_SMEM_IMEM
endif

ifeq ($(QCOM_MMU_SMEM_IRAM),1)
GLOBAL_DEFINES += QCOM_MMU_SMEM_IRAM
endif

include $(LOCAL_DIR)/tools/Makefile

include make/module.mk
