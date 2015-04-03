SUBDIRS = linux_test automation/WindowPlugin automation/FritzBoxPlugin


UNAME_M = $(shell uname -m)
ifeq ($(UNAME_M),armv6l)
	SUBDIRS += rpi/gertboard
endif

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@
