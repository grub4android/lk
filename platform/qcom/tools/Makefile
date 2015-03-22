APPSBOOTHEADER_DIR  := $(BUILDDIR)
HOST_CC := gcc
QCOM_TOOLS_DIR := $(LOCAL_DIR)

# select build dep
ifeq ($(QCOM_ENABLE_EMMC),1)
	EXTRA_BUILDDEPS += emmc_appsboot.mbn
else
	ifneq ($(QCOM_ENABLE_NANDWRITE),1)
		EXTRA_BUILDDEPS += appsboot.mbn
	endif
endif

# APPSBOOT
appsboot.mbn: appsboothd.mbn $(OUTBIN)
	cp $(OUTBIN) $(APPSBOOTHEADER_DIR)/appsboot.raw
	cat $(APPSBOOTHEADER_DIR)/appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/appsboot.mbn
	rm -f $(APPSBOOTHEADER_DIR)/appsboothd.mbn

appsboothd.mbn: mkheader $(OUTBIN)
	$(BUILDDIR)/mkheader $(OUTBIN) $(APPSBOOTHEADER_DIR)/appsboothd.mbn

# EMMCBOOT
emmc_appsboot.mbn: emmc_appsboothd.mbn $(OUTBIN)
	cat $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn $(OUTBIN) > $(APPSBOOTHEADER_DIR)/emmc_appsboot.mbn
	rm -f $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn

emmc_appsboothd.mbn: mkheader $(OUTBIN)
	$(BUILDDIR)/mkheader $(OUTBIN) $(APPSBOOTHEADER_DIR)/emmc_appsboothd.mbn

# MKHEADER
mkheader: $(QCOM_TOOLS_DIR)/tools/mkheader.c
	@mkdir -p $(BUILDDIR)
	${HOST_CC} -DMEMBASE=$$(( $(MEMBASE) + $(KERNEL_LOAD_OFFSET) )) $(QCOM_TOOLS_DIR)/tools/mkheader.c -o $(BUILDDIR)/mkheader
