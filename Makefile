PROG := latex-fast-compile
PREFIX ?= /usr/local

$(PROG): $(PROG).c

.PHONY: install
install: $(PROG)
	install -Dm755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -Dm644 man/$(PROG).1 $(DESTDIR)$(PREFIX)/share/man/man1/$(PROG).1

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	$(RM) $(DESTDIR)$(PREFIX)/share/man/man1/$(PROG).1

.PHONY: clean
clean:
	$(RM) $(PROG)
