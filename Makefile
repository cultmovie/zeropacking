.PHONY : all clean
CC ?= gcc
CFLAGS = -g -O2 -Wall 
LDFLAGS = 

BIN := ./bin
TARGET_PACK := $(BIN)/packz
TARGET_UNPACK := $(BIN)/unpackz
TARGET_UNPACKM := $(BIN)/unpackzm

all: build $(TARGET_PACK) $(TARGET_UNPACK) $(TARGET_UNPACKM)

build:
	-mkdir -p bin

SRC = packz.c
$(TARGET_PACK) : $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

UNPACK_SRC = unpackz.c
$(TARGET_UNPACK) : $(UNPACK_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

UNPACK_SRC = unpackzm.c
$(TARGET_UNPACKM) : $(UNPACK_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean : 
	-rm -rf $(BIN)
