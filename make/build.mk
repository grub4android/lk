# use linker garbage collection, if requested
ifeq ($(WITH_LINKER_GC),1)
GLOBAL_COMPILEFLAGS += -ffunction-sections -fdata-sections
GLOBAL_LDFLAGS += --gc-sections
endif

$(OUTBIN): $(OUTELF)
	@echo generating image: $@
	$(NOECHO)$(SIZE) $<
	$(NOECHO)$(OBJCOPY) -O binary $< $@

$(OUTELF).hex: $(OUTELF)
	@echo generating hex file: $@
	$(NOECHO)$(OBJCOPY) -O ihex $< $@

ifeq ($(ENABLE_TRUSTZONE), 1)
$(OUTELF): $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LINKER_SCRIPT) $(OUTPUT_TZ_BIN)
	@echo linking $@
	$(NOECHO)$(SIZE) -t --common $(sort $(ALLMODULE_OBJS))
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(OUTPUT_TZ_BIN) $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LIBGCC) -o $@
else
$(OUTELF): $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LINKER_SCRIPT)
	@echo linking $@
	$(NOECHO)$(SIZE) -t --common $(sort $(ALLMODULE_OBJS))
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LIBGCC) -o $@
endif


$(OUTELF).sym: $(OUTELF)
	@echo generating symbols: $@
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) > $@

$(OUTELF).sym.sorted: $(OUTELF)
	@echo generating symbols: $@
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) | sort > $@

$(OUTELF).lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -d $< | $(CPPFILT) > $@

$(OUTELF).debug.lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -S $< | $(CPPFILT) > $@

$(OUTELF).size: $(OUTELF)
	@echo generating size map: $@
	$(NOECHO)$(NM) -S --size-sort $< > $@

ifeq ($(ENABLE_TRUSTZONE), 1)
$(OUTPUT_TZ_BIN): $(INPUT_TZ_BIN)
	@echo generating TZ output from TZ input
	$(NOECHO)$(OBJCOPY) -I binary -B arm -O elf32-littlearm $(INPUT_TZ_BIN) $(OUTPUT_TZ_BIN)
endif

$(OUTELF_STRIP): $(OUTELF)
	@echo generating stripped elf: $@
	$(NOECHO)$(STRIP) -S $< -o $@

#include arch/$(ARCH)/compile.mk

