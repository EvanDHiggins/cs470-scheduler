CC = clang++
OUT = scheduler
CFLAGS = -std=c++11 -g -O0

default: *.cpp
	$(CC) -o $(OUT) $^ $(CFLAGS) 

.PHONY: clean
clean:
	rm $(OUT)
	rm output.txt
