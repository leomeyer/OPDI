SUBDIRS = linux_test automation/WindowPlugin automation/FritzBoxPlugin

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@
