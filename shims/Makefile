CC = x86_64-w64-mingw32-gcc
CFLAGS = -O2 -Wall -Wextra

BINS = powershell.exe cmdwait.exe uritest.exe cmd.exe

.PHONY: all clean prebuilt install

all: $(BINS)

powershell.exe: powershell_shim.c
	$(CC) $(CFLAGS) -o $@ $<

cmdwait.exe: cmdwait.c
	$(CC) $(CFLAGS) -o $@ $<

cmd.exe: cmdwait.exe
	cp cmdwait.exe cmd.exe

uritest.exe: uritest.c
	$(CC) $(CFLAGS) -o $@ $<

prebuilt: all
	mkdir -p prebuilt
	cp -f $(BINS) prebuilt/

clean:
	rm -f $(BINS)

install: all
	@test -n "$$WINEPREFIX" || { echo "Error: WINEPREFIX environment variable is not set"; exit 1; }
	mkdir -p "$$WINEPREFIX/drive_c/windows/system32"
	mkdir -p "$$WINEPREFIX/drive_c/windows/cmdwait"
	cp -f powershell.exe "$$WINEPREFIX/drive_c/windows/system32/powershell.exe"
	cp -f cmd.exe "$$WINEPREFIX/drive_c/windows/cmdwait/cmd.exe"
	@echo "Shims successfully installed to $$WINEPREFIX"
