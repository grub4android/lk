LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# can override this in local.mk
ENABLE_THUMB?=true

# default to the regular arm subarch
SUBARCH := arm

GLOBAL_DEFINES += \
	ARM_CPU_$(ARM_CPU)=1

# do set some options based on the cpu core
HANDLED_CORE := false
ifeq ($(ARM_CPU),cortex-m3)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M3=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-m4)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M4=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-m4f)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_M4=1 \
	ARM_CPU_CORTEX_M4F=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_VFP=1 \
	__FPU_PRESENT=1
GLOBAL_COMPILEFLAGS += -mcpu=cortex-m4 -mfloat-abi=softfp
HANDLED_CORE := true
ENABLE_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-a8)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
GLOBAL_COMPILEFLAGS += -mfpu=vfpv3 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-a9)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7A=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),cortex-a9-neon)
GLOBAL_DEFINES += \
	ARM_CPU_CORTEX_A9=1 \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1
GLOBAL_COMPILEFLAGS += -mcpu=cortex-a9
HANDLED_CORE := true
# XXX cannot enable neon right now because compiler generates
# neon code for 64bit integer ops
GLOBAL_COMPILEFLAGS += -mfpu=vfpv3 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),arm1136j-s)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv6=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm1176jzf-s)
GLOBAL_DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv6=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif

ifneq ($(HANDLED_CORE),true)
$(error $(LOCAL_DIR)/rules.mk doesnt have logic for arm core $(ARM_CPU))
endif

THUMBCFLAGS :=
THUMBINTERWORK :=
ifeq ($(ENABLE_THUMB),true)
THUMBCFLAGS := -mthumb -D__thumb__
THUMBINTERWORK := -mthumb-interwork
endif

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)/$(SUBARCH)/include

ifeq ($(SUBARCH),arm)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/asm.S \
	$(LOCAL_DIR)/arm/cache-ops.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/exceptions.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/fpu.c \
	$(LOCAL_DIR)/arm/mmu.c \
	$(LOCAL_DIR)/arm/thread.c \
	$(LOCAL_DIR)/arm/dcc.S

MODULE_ARM_OVERRIDE_SRCS := \
	$(LOCAL_DIR)/arm/arch.c

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=4096

ARCH_OPTFLAGS := -O2
WITH_LINKER_GC ?= 1

# we have a mmu and want the vmm/pmm
WITH_KERNEL_VM=1

KERNEL_LOAD_OFFSET ?= 0
ifeq ($(KERNEL_VM_IDMAPS_ONLY), 1)
GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=0x00100000 \
    KERNEL_ASPACE_SIZE=0xffefffff \
    KERNEL_VM_IDMAPS_ONLY=1

KERNEL_BASE ?= $(MEMBASE)
else
# for arm, have the kernel occupy the entire top 3GB of virtual space,
# but put the kernel itself at 0x80000000.
# this leaves 0x40000000 - 0x80000000 open for kernel space to use.
GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=0x40000000 \
    KERNEL_ASPACE_SIZE=0xc0000000 \
    KERNEL_VM_IDMAPS_ONLY=0

KERNEL_BASE ?= 0x80000000
endif

GLOBAL_DEFINES += \
    KERNEL_BASE=$(KERNEL_BASE) \
    KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET)
endif
ifeq ($(SUBARCH),arm-m)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm-m/arch.c \
	$(LOCAL_DIR)/arm-m/vectab.c \
	$(LOCAL_DIR)/arm-m/start.c \
	$(LOCAL_DIR)/arm-m/exceptions.c \
	$(LOCAL_DIR)/arm-m/thread.c

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/arm-m/CMSIS/Include

# we're building for small binaries
GLOBAL_DEFINES += \
	ARM_ONLY_THUMB=1 \
	ARCH_DEFAULT_STACK_SIZE=1024

ARCH_OPTFLAGS := -Os
WITH_LINKER_GC ?= 1
endif

# try to find the toolchain
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := arm-eabi-
FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
TOOLCHAIN_PREFIX := arm-elf-
FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
TOOLCHAIN_PREFIX := arm-none-eabi-
FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
TOOLCHAIN_PREFIX := arm-linux-gnueabi-
FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)

# Set no stack protection if we found our gnueabi toolchain. We don't
# need it.
#
# Stack protection is default in this toolchain and we get such errors
# final linking stage:
#
# undefined reference to `__stack_chk_guard'
# undefined reference to `__stack_chk_fail'
# undefined reference to `__stack_chk_guard'
#
ifneq (,$(findstring arm-linux-gnueabi-,$(FOUNDTOOL)))
        GLOBAL_COMPILEFLAGS += -fno-stack-protector
endif

endif
endif
endif
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set TOOLCHAIN_PREFIX or add it to your path)
endif
endif
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

GLOBAL_COMPILEFLAGS += $(THUMBINTERWORK)

# make sure some bits were set up
MEMVARS_SET := 0
ifneq ($(MEMBASE),)
MEMVARS_SET := 1
endif
ifneq ($(MEMSIZE),)
MEMVARS_SET := 1
endif
ifeq ($(MEMVARS_SET),0)
$(error missing MEMBASE or MEMSIZE variable, please set in target rules.mk)
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld \
	$(BUILDDIR)/system-twosegment.ld

# rules for generating the linker scripts

$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@

$(BUILDDIR)/system-twosegment.ld: $(LOCAL_DIR)/system-twosegment.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@

# arm specific script to try to guess stack usage
$(OUTELF).stack: LOCAL_DIR:=$(LOCAL_DIR)
$(OUTELF).stack: $(OUTELF).lst
	$(NOECHO)echo generating stack usage $@
	$(NOECHO)$(LOCAL_DIR)/stackusage < $< | sort -n -k 1 -r > $@

EXTRA_BUILDDEPS += $(OUTELF).stack

include make/module.mk
