bin_PROGRAMS = tmwserv
tmwserv_SOURCES = main.cpp \
		  accounthandler.h \
		  accounthandler.cpp \
		  connectionhandler.h \
		  connectionhandler.cpp \
		  debug.h \
		  debug.cpp \
		  defines.h \
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
		  packet.cpp \
		  sqlite/SQLiteWrapper.cpp \
		  sqlite/SQLiteWrapper.h \
		  log.h \
		  log.cpp

# set the include path found by configure
INCLUDES= $(all_includes) $(SQLITE_CFLAGS)
LIBS = $(SQLITE_LIBS)

# the library search path.
tmwserv_LDFLAGS = $(all_libraries) -lSDL_net `sdl-config --libs` `pkg-config --libs libxml-2.0` -lphysfs
tmwserv_CXXFLAGS = -g -Wall `sdl-config --cflags` `pkg-config --cflags libxml-2.0` -fno-inline
#tmwserv_LDADD = $(LIBSDL_LIBS) -lphysfs
tmwserv_TARGET = ../tmwserv
