CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
TARGET := build/test_ppg_ibi_basic

.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

$(TARGET): src/ppg_ibi.c tests/test_ppg_ibi_basic.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_basic.c -o $(TARGET)

clean:
	rm -rf build
