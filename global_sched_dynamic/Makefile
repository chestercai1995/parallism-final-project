
CCP = g++
CC = gcc
CCPFLAGS = -Wall -lpapi -std=c++11 -Wno-write-strings
CCFLAGS = -Wall -lpapi -Wno-write-strings

SRCS = $(wildcard *.c)
BINDIR = bin
#INCLUDE = -I/misc/pool6/aniket/parallelism-final-project/papi-5.6.0/src
#LIBS = /misc/pool6/aniket/parallelism-final-project/papi-5.6.0/src
INCLUDE = -I/opt/apps/papi/5.6.0/include 
LIBS = /opt/apps/papi/5.6.0/lib

all: c_objs cpp_objs
	./cp.sh

c_objs: $(patsubst %.c, %, $(wildcard *.c))

cpp_objs: $(patsubst %.cpp, %, $(wildcard *.cpp))


%: %.c Makefile
	$(CC) $(INCLUDE) -L$(LIBS) -o $(BINDIR)/$@ $< $(CCFLAGS)

%: %.cpp Makefile
	$(CCP) $(INCLUDE) -L$(LIBS) -o $(BINDIR)/$@ $< $(CCPFLAGS)

clean:
	rm -f $(BINDIR)/*
