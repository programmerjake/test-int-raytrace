.PHONY: all clean

all: test-int-raytrace

CXXFLAGS = -O3 $(shell sdl-config --cflags)
LDLIBS = $(shell sdl-config --libs)

main.o: main.cpp

test-int-raytrace: main.o
	$(CXX) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f *.o test-int-raytrace