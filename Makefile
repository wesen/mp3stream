#CC=gcc-3.0
CFLAGS += -Wall -g
CXXFLAGS += -Wall -g

OBJS := AudioCard.o \
	Streamer.o \
	Url.o \
	LameEncoder.o

all: mp3stream

urlTest: Url.o urlTest.o
	$(CXX) -o $@ Url.o urlTest.o

mp3stream: $(OBJS) mp3stream.o
	$(CXX) -o $@ mp3stream.o $(OBJS) -lmp3lame

clean:
	- rm -rf *.o mp3stream