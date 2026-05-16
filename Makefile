CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude

TARGET_BASIC := build/test_ppg_ibi_basic
TARGET_SYNTH := build/test_ppg_ibi_synthetic

.PHONY: all test clean

all: test

test: $(TARGET_BASIC) $(TARGET_SYNTH)
	$(TARGET_BASIC)
	$(TARGET_SYNTH)

$(TARGET_BASIC): src/ppg_ibi.c tests/test_ppg_ibi_basic.c include/ppg_ibi.h | build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_basic.c -o $(TARGET_BASIC)

$(TARGET_SYNTH): src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c include/ppg_ibi.h | build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c -o $(TARGET_SYNTH)

build:
	mkdir -p build

clean:
	rm -rf build
