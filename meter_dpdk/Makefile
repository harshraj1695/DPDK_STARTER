APP := main
SRC := main.c
CC := gcc
PKGCONF := pkg-config

CFLAGS  ?= -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS ?= $(shell $(PKGCONF) --libs libdpdk)

# ===============================
# Build target
# ===============================
$(APP): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(LDFLAGS)

# ===============================
# Debug build (GDB)
# ===============================
debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="-O0 -g $(shell $(PKGCONF) --cflags libdpdk)" \
	        LDFLAGS="$(shell $(PKGCONF) --libs libdpdk)"

# ===============================
# Address Sanitizer build
# ===============================
asan:
	$(MAKE) clean
	$(MAKE) CFLAGS="-O1 -g -fsanitize=address $(shell $(PKGCONF) --cflags libdpdk)" \
	        LDFLAGS="-fsanitize=address $(shell $(PKGCONF) --libs libdpdk)"

# ===============================
# Clean
# ===============================
clean:
	rm -f $(APP)
