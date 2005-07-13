#!/bin/bash
g++ -I/usr/include/SDL -D_REENTRANT -o client \
	src/client.cpp src/messageout.cpp src/packet.cpp \
	-L/usr/lib -lSDL_net -lSDL -lpthread
