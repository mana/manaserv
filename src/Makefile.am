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
		  object.cpp \
		  dal/dataprovider.h \
		  dal/dataprovider.cpp \
		  dal/dataproviderfactory.h \
		  dal/dataproviderfactory.cpp \
		  dal/recordset.h \
		  dal/recordset.cpp \
		  dal/dalexcept.h \
		  dalstorage.h \
		  dalstoragesql.h \
		  dalstorage.cpp \
		  utils/singleton.h \
		  utils/cipher.h \
		  utils/cipher.cpp \
		  utils/logger.h \
		  utils/logger.cpp

if BUILD_SQLITE
tmwserv_SOURCES += dal/sqlitedataprovider.h \
		   dal/sqlitedataprovider.cpp
endif

if BUILD_POSTGRE
tmwserv_SOURCES += dal/pqdataprovider.h \
		   dal/pqdataprovider.cpp
endif

if BUILD_MYSQL
tmwserv_SOURCES += dal/mysqldataprovider.h \
		   dal/mysqldataprovider.cpp
endif

# set the include path found by configure
INCLUDES= $(all_includes) 
LIBS = $(SQLITE_LIBS) $(MYSQL_LIBS) $(POSTGRE_LIBS)

# the library search path.
tmwserv_LDFLAGS = $(all_libraries) \
				  -lSDL_net `sdl-config --libs` \
				  `pkg-config --libs libxml-2.0` \
				  -lphysfs \
				  -lcrypto
tmwserv_CXXFLAGS = -g -Wall `sdl-config --cflags` `pkg-config --cflags libxml-2.0` -fno-inline $(SCRIPT_CFLAGS) $(POSTGRE_CFLAGS) $(SQLITE_CFLAGS)

#tmwserv_LDADD = $(LIBSDL_LIBS) -lphysfs
tmwserv_LDADD = $(SCRIPT_LIBS)
tmwserv_TARGET = ../tmwserv

