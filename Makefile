PROG := latex-fast-compile
PREFIX ?= /usr/local

$(PROG): $(PROG).c

.PHONY: install
install: $(PROG)
	install -Dm755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(PROG)
