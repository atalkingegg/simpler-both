.phony := all clean

PROGS := simpler-both
OBJS := LibCamera.o main.o

CXXFLAGS := -Wall -g -std=c++17 -I/usr/local/include/libcamera
CXXLDFLAGS := -L/usr/local/lib -lcamera-base -lcamera -lopencv_core -lopencv_highgui

## future no opencv version
# NCVLDFLAGS := -L/usr/local/lib -lcamera-base -lcamera

all : ${PROGS}

LibCamera.o: LibCamera.cpp LibCamera.h flat_libcamera.h
	g++ ${CXXFLAGS} -c -o $@ $<

main.o: main.cpp LibCamera.h flat_libcamera.h
	g++ ${CXXFLAGS} -c -o $@ $<

simpler-both: ${OBJS}
	g++ -o $@ $^ ${CXXLDFLAGS}

clean:
	rm -f ${PROGS} ${OBJS}
