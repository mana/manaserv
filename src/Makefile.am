# $Id$


bin_PROGRAMS = tmwserv

tmwserv_SOURCES = main.cpp \
				  accounthandler.cpp \
				  connectionhandler.cpp \
				  debug.cpp \
				  defines.h \
				  messagehandler.cpp \
				  messagein.cpp \
				  messageout.cpp \
				  netcomputer.cpp \
				  netsession.cpp \
				  packet.cpp \
				  skill.cpp \
				  storage.cpp \
				  account.cpp \
				  object.cpp \
				  dalstorage.cpp \
				  dal/dataprovider.cpp \
				  dal/dataproviderfactory.cpp \
				  dal/recordset.cpp \
				  utils/cipher.cpp \
				  utils/logger.cpp

if BUILD_MYSQL
tmwserv_SOURCES += dal/mysqldataprovider.cpp
endif

if BUILD_POSTGRESQL
tmwserv_SOURCES += dal/pqdataprovider.cpp
endif

if BUILD_SQLITE
tmwserv_SOURCES += dal/sqlitedataprovider.cpp
endif

if BUILD_SQUIRREL
tmwserv_SOURCES += script.cpp \
				   script-squirrel.cpp
endif

tmwserv_CXXFLAGS = -Wall -fno-inline
tmwserv_TARGET = tmwserv
