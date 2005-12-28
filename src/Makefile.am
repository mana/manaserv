# $Id$

bin_PROGRAMS = tmwclient tmwserv

tmwclient_SOURCES = client.cpp \
		    defines.h \
		    messageout.h \
		    messageout.cpp \
		    packet.h \
		    packet.cpp

tmwserv_SOURCES = main.cpp \
		  configuration.h \
		  configuration.cpp \
		  accounthandler.h \
		  accounthandler.cpp \
		  chathandler.h \
		  chathandler.cpp \
		  connectionhandler.h \
		  connectionhandler.cpp \
		  gamehandler.h \
		  gamehandler.cpp \
		  state.h \
		  state.cpp \
		  debug.h \
		  debug.cpp \
		  defines.h \
		  items.h \
		  items.cpp \
		  map.cpp \
		  map.h \
		  mapmanager.h \
		  mapmanager.cpp \
		  mapreader.cpp \
		  mapreader.h \
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
		  resourcemanager.cpp \
		  resourcemanager.h \
		  storage.h \
		  storage.cpp \
		  account.h \
		  account.cpp \
		  object.h \
		  object.cpp \
		  being.h \
		  being.cpp \
		  dalstorage.h \
		  dalstorage.cpp \
		  dal/dalexcept.h \
		  dal/dataprovider.h \
		  dal/dataprovider.cpp \
		  dal/dataproviderfactory.h \
		  dal/dataproviderfactory.cpp \
		  dal/recordset.h \
		  dal/recordset.cpp \
		  utils/base64.h \
		  utils/base64.cpp \
		  utils/countedptr.h \
		  utils/functors.h \
		  utils/singleton.h \
		  utils/cipher.h \
		  utils/cipher.cpp \
		  utils/logger.h \
		  utils/logger.cpp \
		  utils/slangsfilter.h \
		  utils/slangsfilter.cpp

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

if BUILD_RUBY
tmwserv_SOURCES += bindings_wrap.cpp
bindings_wrap.cpp: bindings.i
	$(SWIG) $(SWIGFLAGS) -o bindings_wrap.cpp bindings.i
endif

tmwserv_CXXFLAGS = -Wall -fno-inline


# do not remove the 'dot', it is a hint for Automake.
# it causes a prefix ordering of directories.
SUBDIRS = . tests
