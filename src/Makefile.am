bin_PROGRAMS = tmwserv
tmwserv_SOURCES = main.cpp \
		  connectionhandler.h \
		  connectionhandler.cpp \
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
tmwserv_LDFLAGS = $(all_libraries) $(LIBSDL_RPATH) `pkg-config --libs libxml-2.0`
tmwserv_CXXFLAGS = -g -Wall $(LIBSDL_CFLAGS) `pkg-config --cflags libxml-2.0` -fno-inline
tmwserv_LDADD = $(LIBSDL_LIBS) -lphysfs
tmwserv_TARGET = ../tmwserv
