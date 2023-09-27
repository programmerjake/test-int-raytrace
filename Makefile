.PHONY: all

all: test-int-raytrace

CXXFLAGS = $(shell sdl-config --cflags)
LDLIBS = $(shell sdl-config --libs)

main.o: main.cpp

test-int-raytrace: main.o
	$(CXX) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@
