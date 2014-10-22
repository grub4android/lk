LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/openssl

GLOBAL_INCLUDES += \
	$(LOCAL_DIR) \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += $(TARGET_XRES)
GLOBAL_DEFINES += $(TARGET_YRES)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/smem.c \
	$(LOCAL_DIR)/smem_ptable.c \
	$(LOCAL_DIR)/jtag_hook.S \
	$(LOCAL_DIR)/jtag.c \
	$(LOCAL_DIR)/partition_parser.c \
	$(LOCAL_DIR)/hsusb.c \
	$(LOCAL_DIR)/boot_stats.c

ifeq ($(ENABLE_SMD_SUPPORT),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/rpm-smd.c \
	$(LOCAL_DIR)/smd.c \
	$(LOCAL_DIR)/regulator.c
endif

ifeq ($(ENABLE_SDHCI_SUPPORT),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/sdhci.c \
	$(LOCAL_DIR)/sdhci_msm.c \
	$(LOCAL_DIR)/mmc_sdhci.c \
	$(LOCAL_DIR)/mmc_wrapper.c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/mmc.c
endif

ifeq ($(ENABLE_VERIFIED_BOOT),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/boot_verifier.c
endif

ifeq ($(ENABLE_2NDSTAGE_BOOT),1)
ifeq ($(DISPLAY_2NDSTAGE_WIDTH),)
$(error DISPLAY: No width specified.)
else
GLOBAL_DEFINES += DISPLAY_2NDSTAGE_WIDTH=$(DISPLAY_2NDSTAGE_WIDTH)
endif

ifeq ($(DISPLAY_2NDSTAGE_HEIGHT),)
$(error DISPLAY: No height specified.)
else
GLOBAL_DEFINES += DISPLAY_2NDSTAGE_HEIGHT=$(DISPLAY_2NDSTAGE_HEIGHT)
endif

ifeq ($(DISPLAY_2NDSTAGE_BPP),)
$(error DISPLAY: No bpp specified.)
else
GLOBAL_DEFINES += DISPLAY_2NDSTAGE_BPP=$(DISPLAY_2NDSTAGE_BPP)
endif

ifneq ($(DISPLAY_2NDSTAGE_FBADDR),)
GLOBAL_DEFINES += DISPLAY_2NDSTAGE_FBADDR=$(DISPLAY_2NDSTAGE_FBADDR)
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/display_2ndstage.c
endif

ifeq ($(PLATFORM),msm8x60)
	MODULE_SRCS += $(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/crypto_eng.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/lcdc.c \
			$(LOCAL_DIR)/mddi.c \
			$(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/mdp4.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/timer.c \
			$(LOCAL_DIR)/nand.c
endif

ifeq ($(PLATFORM),msm8960)
	MODULE_SRCS += $(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/mdp4.c \
			$(LOCAL_DIR)/crypto4_eng.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock-local.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/lvds.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/timer.c \
			$(LOCAL_DIR)/mdp_lcdc.c \
			$(LOCAL_DIR)/nand.c \
			$(LOCAL_DIR)/dload_util.c
endif

ifeq ($(PLATFORM),msm8974)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/mdp5.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/mipi_dsi_autopll.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/dload_util.c \
			$(LOCAL_DIR)/edp.c \
			$(LOCAL_DIR)/edp_util.c \
			$(LOCAL_DIR)/edp_aux.c \
			$(LOCAL_DIR)/edp_phy.c
endif

ifeq ($(PLATFORM),msm8226)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/mdp5.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/mipi_dsi_autopll.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
            $(LOCAL_DIR)/certificate.c \
            $(LOCAL_DIR)/image_verify.c \
            $(LOCAL_DIR)/crypto_hash.c \
            $(LOCAL_DIR)/crypto5_eng.c \
            $(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/dload_util.c \
			$(LOCAL_DIR)/shutdown_detect.c
endif

ifeq ($(PLATFORM),msm8916)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
		$(LOCAL_DIR)/qtimer.c \
		$(LOCAL_DIR)/qtimer_mmap.c \
		$(LOCAL_DIR)/interrupts.c \
		$(LOCAL_DIR)/clock.c \
		$(LOCAL_DIR)/clock_pll.c \
		$(LOCAL_DIR)/clock_lib2.c \
		$(LOCAL_DIR)/uart_dm.c \
		$(LOCAL_DIR)/board.c \
		$(LOCAL_DIR)/spmi.c \
		$(LOCAL_DIR)/bam.c \
		$(LOCAL_DIR)/scm.c \
		$(LOCAL_DIR)/qpic_nand.c \
		$(LOCAL_DIR)/dload_util.c \
		$(LOCAL_DIR)/gpio.c \
		$(LOCAL_DIR)/dev_tree.c \
		$(LOCAL_DIR)/mdp5.c \
		$(LOCAL_DIR)/display.c \
		$(LOCAL_DIR)/mipi_dsi.c \
		$(LOCAL_DIR)/mipi_dsi_phy.c \
		$(LOCAL_DIR)/mipi_dsi_autopll.c \
		$(LOCAL_DIR)/shutdown_detect.c \
		$(LOCAL_DIR)/certificate.c \
		$(LOCAL_DIR)/image_verify.c \
		$(LOCAL_DIR)/crypto_hash.c \
		$(LOCAL_DIR)/crypto5_eng.c \
		$(LOCAL_DIR)/crypto5_wrapper.c \
		$(LOCAL_DIR)/i2c_qup.c

endif


ifeq ($(PLATFORM),msm8610)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
    MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
            $(LOCAL_DIR)/qtimer.c \
            $(LOCAL_DIR)/qtimer_mmap.c \
            $(LOCAL_DIR)/interrupts.c \
            $(LOCAL_DIR)/clock.c \
            $(LOCAL_DIR)/clock_pll.c \
            $(LOCAL_DIR)/clock_lib2.c \
            $(LOCAL_DIR)/uart_dm.c \
            $(LOCAL_DIR)/board.c \
            $(LOCAL_DIR)/display.c \
            $(LOCAL_DIR)/mipi_dsi.c \
            $(LOCAL_DIR)/mipi_dsi_phy.c \
            $(LOCAL_DIR)/mdp3.c \
            $(LOCAL_DIR)/spmi.c \
            $(LOCAL_DIR)/bam.c \
            $(LOCAL_DIR)/qpic_nand.c \
            $(LOCAL_DIR)/dev_tree.c \
            $(LOCAL_DIR)/scm.c \
            $(LOCAL_DIR)/gpio.c \
            $(LOCAL_DIR)/certificate.c \
            $(LOCAL_DIR)/image_verify.c \
            $(LOCAL_DIR)/crypto_hash.c \
            $(LOCAL_DIR)/crypto5_eng.c \
            $(LOCAL_DIR)/crypto5_wrapper.c \
            $(LOCAL_DIR)/dload_util.c \
            $(LOCAL_DIR)/shutdown_detect.c
endif

ifeq ($(PLATFORM),apq8084)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
    MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
            $(LOCAL_DIR)/qtimer.c \
            $(LOCAL_DIR)/qtimer_mmap.c \
            $(LOCAL_DIR)/interrupts.c \
            $(LOCAL_DIR)/clock.c \
            $(LOCAL_DIR)/clock_pll.c \
            $(LOCAL_DIR)/clock_lib2.c \
            $(LOCAL_DIR)/uart_dm.c \
            $(LOCAL_DIR)/board.c \
            $(LOCAL_DIR)/mdp5.c \
            $(LOCAL_DIR)/display.c \
            $(LOCAL_DIR)/mipi_dsi.c \
            $(LOCAL_DIR)/mipi_dsi_phy.c \
            $(LOCAL_DIR)/mipi_dsi_autopll.c \
            $(LOCAL_DIR)/mdss_hdmi.c \
            $(LOCAL_DIR)/hdmi_pll_28nm.c \
            $(LOCAL_DIR)/spmi.c \
            $(LOCAL_DIR)/bam.c \
            $(LOCAL_DIR)/qpic_nand.c \
            $(LOCAL_DIR)/dev_tree.c \
            $(LOCAL_DIR)/gpio.c \
            $(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/ufs.c \
			$(LOCAL_DIR)/utp.c \
			$(LOCAL_DIR)/uic.c \
			$(LOCAL_DIR)/ucs.c \
			$(LOCAL_DIR)/ufs_hci.c \
			$(LOCAL_DIR)/dme.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/edp.c \
			$(LOCAL_DIR)/edp_util.c \
			$(LOCAL_DIR)/edp_aux.c \
			$(LOCAL_DIR)/edp_phy.c

endif

ifeq ($(PLATFORM),msm7x27a)
	MODULE_SRCS += $(LOCAL_DIR)/uart.c \
			$(LOCAL_DIR)/nand.c \
			$(LOCAL_DIR)/proc_comm.c \
			$(LOCAL_DIR)/mdp3.c \
			$(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/crypto_eng.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/timer.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/mdp_lcdc.c \
			$(LOCAL_DIR)/spi.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/board.c
endif

ifeq ($(PLATFORM),msm7k)
	MODULE_SRCS += $(LOCAL_DIR)/uart.c \
			$(LOCAL_DIR)/nand.c \
			$(LOCAL_DIR)/proc_comm.c \
			$(LOCAL_DIR)/lcdc.c \
			$(LOCAL_DIR)/mddi.c \
			$(LOCAL_DIR)/timer.c
endif

ifeq ($(PLATFORM),msm7x30)
	MODULE_SRCS += $(LOCAL_DIR)/crypto_eng.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/uart.c \
			$(LOCAL_DIR)/nand.c \
			$(LOCAL_DIR)/proc_comm.c \
			$(LOCAL_DIR)/lcdc.c \
			$(LOCAL_DIR)/mddi.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/timer.c
endif

ifeq ($(PLATFORM),mdm9x15)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/nand.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/timer.c
endif

ifeq ($(PLATFORM),mdm9x25)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c
endif

ifeq ($(PLATFORM),mdm9x35)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/qmp_usb30_phy.c
endif

ifeq ($(PLATFORM),msmzirc)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/qmp_usb30_phy.c \
			$(LOCAL_DIR)/qusb2_phy.c
endif

ifeq ($(PLATFORM),fsm9900)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/dload_util.c
endif

ifeq ($(PLATFORM),fsm9010)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/dload_util.c
endif

ifeq ($(PLATFORM),msm8994)
GLOBAL_DEFINES += DISPLAY_TYPE_MDSS=1
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/ufs.c \
			$(LOCAL_DIR)/utp.c \
			$(LOCAL_DIR)/uic.c \
			$(LOCAL_DIR)/ucs.c \
			$(LOCAL_DIR)/ufs_hci.c \
			$(LOCAL_DIR)/dme.c \
			$(LOCAL_DIR)/qmp_usb30_phy.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/qusb2_phy.c \
			$(LOCAL_DIR)/mdp5.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/mipi_dsi_autopll.c \
			$(LOCAL_DIR)/mipi_dsi_autopll_20nm.c
endif

ifeq ($(PLATFORM),msm8909)
	MODULE_SRCS += $(LOCAL_DIR)/qgic.c \
			$(LOCAL_DIR)/qtimer.c \
			$(LOCAL_DIR)/qtimer_mmap.c \
			$(LOCAL_DIR)/interrupts.c \
			$(LOCAL_DIR)/clock.c \
			$(LOCAL_DIR)/clock_pll.c \
			$(LOCAL_DIR)/clock_lib2.c \
			$(LOCAL_DIR)/uart_dm.c \
			$(LOCAL_DIR)/board.c \
			$(LOCAL_DIR)/spmi.c \
			$(LOCAL_DIR)/bam.c \
			$(LOCAL_DIR)/qpic_nand.c \
			$(LOCAL_DIR)/scm.c \
			$(LOCAL_DIR)/dev_tree.c \
			$(LOCAL_DIR)/gpio.c \
			$(LOCAL_DIR)/crypto_hash.c \
			$(LOCAL_DIR)/crypto5_eng.c \
			$(LOCAL_DIR)/crypto5_wrapper.c \
			$(LOCAL_DIR)/dload_util.c \
			$(LOCAL_DIR)/shutdown_detect.c \
			$(LOCAL_DIR)/certificate.c \
			$(LOCAL_DIR)/image_verify.c \
			$(LOCAL_DIR)/i2c_qup.c \
			$(LOCAL_DIR)/mdp3.c \
			$(LOCAL_DIR)/display.c \
			$(LOCAL_DIR)/mipi_dsi.c \
			$(LOCAL_DIR)/mipi_dsi_phy.c \
			$(LOCAL_DIR)/mipi_dsi_autopll.c
endif

ifeq ($(ENABLE_BOOT_CONFIG_SUPPORT), 1)
	MODULE_SRCS += \
		$(LOCAL_DIR)/boot_device.c
endif

ifeq ($(ENABLE_USB30_SUPPORT),1)
	MODULE_SRCS += \
		$(LOCAL_DIR)/usb30_dwc.c \
		$(LOCAL_DIR)/usb30_dwc_hw.c \
		$(LOCAL_DIR)/usb30_udc.c \
		$(LOCAL_DIR)/usb30_wrapper.c
endif

include make/module.mk

include target/$(TARGET)/tools/makefile
EXTRA_BUILDDEPS += APPSBOOTHEADER