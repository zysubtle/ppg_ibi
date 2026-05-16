CC := cc
CFLAGS := -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
BASIC := build/test_ppg_ibi_basic
SYNTH := build/test_ppg_ibi_synthetic
EVAL := build/ppg_ibi_eval

.PHONY: test clean

test: $(BASIC) $(SYNTH) $(EVAL)
	./$(BASIC)
	./$(SYNTH)
	./$(EVAL) --self-test

$(BASIC): src/ppg_ibi.c tests/test_ppg_ibi_basic.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_basic.c -lm -o $(BASIC)

$(SYNTH): src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c -lm -o $(SYNTH)

$(EVAL): src/ppg_ibi.c tools/ppg_ibi_eval.c include/ppg_ibi.h
	mkdir -p build
	$(CC) $(CFLAGS) src/ppg_ibi.c tools/ppg_ibi_eval.c -lm -o $(EVAL)

clean:
	rm -rf build
