#CC=gcc-3.0
CFLAGS += -Wall -g
CXXFLAGS += -Wall -g

OBJS := AudioCard.o \
	Streamer.o \
	Url.o \
	LameEncoder.o

all: lame-test urlTest

urlTest: Url.o urlTest.o
	$(CXX) -o $@ Url.o urlTest.o

mp3stream: $(OBJS) mp3stream.o
	$(CXX) -o $@ mp3stream.o $(OBJS)

lame-test: $(OBJS) lame-test.o
	$(CXX) -o $@ lame-test.o $(OBJS) -lmp3lame

clean:
	- rm -rf *.o mp3stream lame-test
