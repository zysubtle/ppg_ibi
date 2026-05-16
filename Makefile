CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
BASIC := build/test_ppg_ibi_basic
SYNTH := build/test_ppg_ibi_synthetic

.PHONY: test clean

test: $(BASIC) $(SYNTH)
	./$(BASIC)
	./$(SYNTH)

$(BASIC): src/ppg_ibi.c tests/test_ppg_ibi_basic.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_basic.c -o $(BASIC)

$(SYNTH): src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c -o $(SYNTH)

clean:
	rm -rf build
