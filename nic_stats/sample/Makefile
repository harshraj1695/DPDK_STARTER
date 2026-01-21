APP = pipeline

SRCS = main.c rx.c parser.c tx.c

CFLAGS += -O3 -I.
LDFLAGS += $(shell pkg-config --libs libdpdk)
CFLAGS += $(shell pkg-config --cflags libdpdk)
CFLAGS += -O0 -g


all:
	$(CC) $(SRCS) -o $(APP) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(APP)
