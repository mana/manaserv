# $Id$

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = tmwclient tmwserv

tmwclient_SOURCES = client.cpp \
		    defines.h \
		    messageout.h \
		    messageout.cpp \
		    messagein.h \
		    messagein.cpp

tmwserv_SOURCES = main.cpp \
		  being.h \
		  being.cpp \
		  account.h \
		  account.cpp \
		  accountclient.h \
		  accountclient.cpp \
		  accounthandler.h \
		  accounthandler.cpp \
		  chathandler.h \
		  chathandler.cpp \
		  chatchannel.h \
		  chatchannel.cpp \
		  chatchannelmanager.h \
		  chatchannelmanager.cpp \
		  configuration.h \
		  configuration.cpp \
		  connectionhandler.h \
		  connectionhandler.cpp \
		  controller.h \
		  controller.cpp \
		  debug.h \
		  debug.cpp \
		  defines.h \
		  gameclient.h \
		  gameclient.cpp \
		  gamehandler.h \
		  gamehandler.cpp \
		  inventory.h \
		  inventory.cpp \
		  item.h \
		  item.cpp \
		  itemhandler.h \
		  itemhandler.cpp \
		  map.h \
		  map.cpp \
		  mapcomposite.h \
		  mapcomposite.cpp \
		  mapmanager.h \
		  mapmanager.cpp \
		  mapreader.cpp \
		  mapreader.h \
		  messagein.h \
		  messagein.cpp \
		  messageout.h \
		  messageout.cpp \
		  netcomputer.h \
		  netcomputer.cpp \
		  object.h \
		  object.cpp \
		  player.h \
		  player.cpp \
		  point.h \
		  properties.h \
		  resourcemanager.cpp \
		  resourcemanager.h \
		  skill.h \
		  skill.cpp \
		  state.h \
		  state.cpp \
		  storage.h \
		  storage.cpp \
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
		  utils/stringfilter.h \
		  utils/stringfilter.cpp \
		  utils/timer.h \
		  utils/timer.cpp

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


# do not remove the 'dot', it is a hint for Automake.
# it causes a prefix ordering of directories.
SUBDIRS = . tests
