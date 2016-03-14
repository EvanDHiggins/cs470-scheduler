CC = clang++
OUT = scheduler
CFLAGS = -std=c++11 -g -O0
VALGRIND_FILE = valgrind.txt

default: *.cpp
	$(CC) -o $(OUT) $^ $(CFLAGS) 

valgrind: default
	valgrind --leak-check=yes --log-file=$(VALGRIND_FILE) ./scheduler input.txt output.txt

.PHONY: clean
clean:
	rm $(OUT)
	rm output.txt
	rm -rf scheduler.dSYM
	rm valgrind.txt
