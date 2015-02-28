-include $(or $(CONFIG),$(ARCH),$(shell uname -m)).mk

override O := $(O:%=$(O:%/=%)/)

SYSROOT = $(addprefix --sysroot=,$(ROOT))

CC = $(CROSS_COMPILE)gcc

CFLAGS = -O2 -g -Wall $(CPUFLAGS)
LDFLAGS = $(SYSROOT)

ALL = memdump mempoke memwrite
ALL := $(ALL:%=$(O)%)
all: $(ALL)

$(O)%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

$(ALL): | $(O)
$(O):
	@mkdir -p $@
