bin_PROGRAMS = tmwserv
tmwserv_SOURCES = main.cpp \
		  connectionhandler.h \
		  connectionhandler.cpp \
		  debug.h \
		  debug.cpp \
		  messagehandler.h \
		  messagehandler.cpp \
		  messagein.h \
		  messagein.cpp \
		  messageout.h \
		  messageout.cpp \
		  netcomputer.h \
		  netcomputer.cpp \
		  netsession.h \
		  netsession.cpp \
		  packet.h \
		  packet.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
tmwserv_LDFLAGS = $(all_libraries) -lSDL_net `sdl-config --libs` `pkg-config --libs libxml-2.0` -lphysfs
tmwserv_CXXFLAGS = -g -Wall `sdl-config --cflags` `pkg-config --cflags libxml-2.0` -fno-inline
#tmwserv_LDADD = $(LIBSDL_LIBS) -lphysfs
tmwserv_TARGET = ../tmwserv
