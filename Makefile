VPATH = src:include

olympos: src/*.cpp
	g++ --std=c++20 -Wall -O3 -pedantic -Wextra -Iinclude $^ -lncurses -o $@

clean:
	rm olympos

