#!/usr/bin/make -f
#
#

CC=gcc
CXX=g++

CFLAGS=-g -Wall -O3
CXXFLAGS=-g -Wall -O3

AR=ar
ARFLAGS=rcs

LDFLAGS=-L .
LIBS=-lshmemutex -lkernel32 -lstdc++

.PHONY: test

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CPPFLAGS) -c $<


SRCS:=shmemutex_core.cpp shmemutex.cpp
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))


all: libshmemutex.a

libshmemutex.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

libshmemutex.dll.a: shmemutex.def Debug/shmemutex.dll
	dlltool --kill-at --def $< --dllname Debug/shmemutex.dll --output-lib $@

libshmemutex.dll.def: Debug/shmemutex.lib
	echo EXPORTS > $@ && \
	nm $< |sed -n -e"s/.* T _//p" >> $@

test: libshmemutex.a
	cd test && $(MAKE)

clean:
	-rm *.o *.a *~
