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
		  log.h \
		  log.cpp \
		  skill.h \
		  skill.cpp \
		  script.h \
		  script.cpp \
		  script-squirrel.h \
		  script-squirrel.cpp \
		  storage.cpp storage.h \
		  account.h \
		  account.cpp \
		  object.h \
		  object.cpp


if BUILD_SQLITE
tmwserv_SOURCES += sqlite/SQLiteWrapper.cpp \
	           sqlite/SQLiteWrapper.h \
		   sqlitestorage.cpp \
		   sqlitestorage.h
endif

# set the include path found by configure
INCLUDES= $(all_includes) $(SQLITE_CFLAGS)
LIBS = $(SQLITE_LIBS)

# the library search path.
tmwserv_LDFLAGS = $(all_libraries) -lSDL_net `sdl-config --libs` `pkg-config --libs libxml-2.0` -lphysfs
tmwserv_CXXFLAGS = -g -Wall `sdl-config --cflags` `pkg-config --cflags libxml-2.0` -fno-inline $(SCRIPT_CFLAGS)
#tmwserv_LDADD = $(LIBSDL_LIBS) -lphysfs
tmwserv_LDADD = $(SCRIPT_LIBS)
tmwserv_TARGET = ../tmwserv
