VPATH = src:include

CXX := g++
# The -MD and -MP flags generate dependencies for .o files.
CXXFLAGS := --std=c++20 -Wall -O3 -pedantic -Wextra -MD -MP -Iinclude
DEBUGFLAGS := --std=c++20 -Wall -pedantic -Wextra -MD -MP --debug -g3 -Iinclude

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPFILES := $(OBJECTS:.o=.d)

olympos: $(OBJECTS)
	g++ $(CXXFLAGS) $^ -lpanelw -lncursesw -o $@

debug: src/*.cpp
	g++ $(DEBUGFLAGS) $^ -lpanelw -lncursesw -o olympos

-include $(DEPFILES)

clean:
	rm olympos
	rm src/*.d
	rm src/*.o

