VPATH = src:include

CXX := g++
# The -MD and -MP flags generate dependencies for .o files.
CXXFLAGS := --std=c++20 -Wall -O3 -pedantic -Wextra -MD -MP -Iinclude 

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPFILES := $(OBJECTS:.o=.d)

olympos: $(OBJECTS)
	g++ $(CXXFLAGS) $^ -lpanel -lncurses -o $@

-include $(DEPFILES)

clean:
	rm olympos

