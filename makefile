# the above include may override LKROOT and LKINC to allow external
# directories to be included in the build
-include lk_inc.mk
GCC_VER_GTE49 := $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.` \>= 4.9 | sed -e 's/\./*100+/g' | bc )
ifeq ($(GCC_VER_GTE49),1)
	CFLAGS += -fdiagnostics-color=always
endif

LKROOT ?= .
LKINC ?=

LKINC := $(LKROOT) $(LKINC)

# vaneer makefile that calls into the engine with lk as the build root
# if we're the top level invocation, call ourselves with additional args
$(MAKECMDGOALS) _top:
	LKROOT=$(LKROOT) LKINC="$(LKINC)" $(MAKE) -rR -f $(LKROOT)/engine.mk $(addprefix -I,$(LKINC)) $(MAKECMDGOALS)

.PHONY: _top
