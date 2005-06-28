# $Id$


bin_PROGRAMS = tmwserv

tmwserv_SOURCES = main.cpp \
				  accounthandler.h \
				  accounthandler.cpp \
				  connectionhandler.h \
				  connectionhandler.cpp \
				  debug.h \
				  debug.cpp \
				  defines.h \
				  items.h \
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
				  storage.h \
				  storage.cpp \
				  account.h \
				  account.cpp \
				  object.h \
				  object.cpp \
				  dalstorage.h \
				  dalstorage.cpp \
				  dal/dalexcept.h \
				  dal/dataprovider.h \
				  dal/dataprovider.cpp \
				  dal/dataproviderfactory.h \
				  dal/dataproviderfactory.cpp \
				  dal/recordset.h \
				  dal/recordset.cpp \
				  utils/functors.h \
				  utils/singleton.h \
				  utils/cipher.h \
				  utils/cipher.cpp \
				  utils/logger.h \
				  utils/logger.cpp \
				  configuration.h \
				  configuration.cpp

if BUILD_MYSQL
tmwserv_SOURCES += dal/mysqldataprovider.h \
				   dal/mysqldataprovider.cpp
endif

if BUILD_POSTGRESQL
tmwserv_SOURCES += dal/pqdataprovider.h \
				   dal/pqdataprovider.cpp
endif

if BUILD_SQLITE
tmwserv_SOURCES += dal/sqlitedataprovider.h \
				   dal/sqlitedataprovider.cpp
endif

if BUILD_SQUIRREL
tmwserv_SOURCES += script.h \
				   script.cpp \
				   script-squirrel.h \
				   script-squirrel.cpp
endif

tmwserv_CXXFLAGS = -Wall -fno-inline


# do not remove the 'dot', it is a hint for Automake.
# it causes a prefix ordering of directories.
SUBDIRS = . tests
