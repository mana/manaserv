# $Id$

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = tmwserv-account tmwserv-game

tmwserv_account_SOURCES = \
	account-server/main-account.cpp \
	configuration.h \
	configuration.cpp \
	defines.h \
	playerdata.hpp \
	point.h \
	resourcemanager.h \
	resourcemanager.cpp \
	skill.h \
	skill.cpp \
	account-server/account.hpp \
	account-server/account.cpp \
	account-server/accountclient.hpp \
	account-server/accountclient.cpp \
	account-server/accounthandler.hpp \
	account-server/accounthandler.cpp \
	account-server/dalstorage.hpp \
	account-server/dalstorage.cpp \
	account-server/dalstoragesql.hpp \
	account-server/serverhandler.hpp \
	account-server/serverhandler.cpp \
	account-server/storage.hpp \
	account-server/storage.cpp \
	chat-server/chathandler.hpp \
	chat-server/chathandler.cpp \
	chat-server/chatchannel.hpp \
	chat-server/chatchannel.cpp \
	chat-server/chatchannelmanager.hpp \
	chat-server/chatchannelmanager.cpp \
	dal/dalexcept.h \
	dal/dataprovider.h \
	dal/dataprovider.cpp \
	dal/dataproviderfactory.h \
	dal/dataproviderfactory.cpp \
	dal/recordset.h \
	dal/recordset.cpp \
	net/connectionhandler.hpp \
	net/connectionhandler.cpp \
	net/messagein.hpp \
	net/messagein.cpp \
	net/messageout.hpp \
	net/messageout.cpp \
	net/netcomputer.hpp \
	net/netcomputer.cpp \
	utils/base64.h \
	utils/base64.cpp \
	utils/countedptr.h \
	utils/functors.h \
	utils/singleton.h \
	utils/logger.h \
	utils/logger.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/timer.h \
	utils/timer.cpp

tmwserv_game_SOURCES = \
	game-server/main-game.cpp \
	configuration.h \
	configuration.cpp \
	controller.h \
	controller.cpp \
	defines.h \
	item.h \
	item.cpp \
	playerdata.hpp \
	point.h \
	resourcemanager.h \
	resourcemanager.cpp \
	skill.h \
	skill.cpp \
	game-server/accountconnection.hpp \
	game-server/accountconnection.cpp \
	game-server/being.hpp \
	game-server/being.cpp \
	game-server/gamehandler.hpp \
	game-server/gamehandler.cpp \
	game-server/inventory.hpp \
	game-server/inventory.cpp \
	game-server/itemmanager.hpp \
	game-server/itemmanager.cpp \
	game-server/map.hpp \
	game-server/map.cpp \
	game-server/mapcomposite.hpp \
	game-server/mapcomposite.cpp \
	game-server/mapmanager.hpp \
	game-server/mapmanager.cpp \
	game-server/mapreader.hpp \
	game-server/mapreader.cpp \
	game-server/object.hpp \
	game-server/object.cpp \
	game-server/player.hpp \
	game-server/player.cpp \
	game-server/state.hpp \
	game-server/state.cpp \
	game-server/testing.cpp \
	game-server/trigger.hpp \
	game-server/trigger.cpp \
	net/connection.hpp \
	net/connection.cpp \
	net/connectionhandler.hpp \
	net/connectionhandler.cpp \
	net/messagein.hpp \
	net/messagein.cpp \
	net/messageout.hpp \
	net/messageout.cpp \
	net/netcomputer.hpp \
	net/netcomputer.cpp \
	utils/base64.h \
	utils/base64.cpp \
	utils/countedptr.h \
	utils/functors.h \
	utils/singleton.h \
	utils/logger.h \
	utils/logger.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/timer.h \
	utils/timer.cpp \
	utils/xml.hpp \
	utils/xml.cpp \
	utils/zlib.hpp \
	utils/zlib.cpp

if BUILD_MYSQL
tmwserv_account_SOURCES += \
	dal/mysqldataprovider.h \
	dal/mysqldataprovider.cpp
endif

if BUILD_POSTGRESQL
tmwserv_account_SOURCES += \
	dal/pqdataprovider.h \
	dal/pqdataprovider.cpp
endif

if BUILD_SQLITE
tmwserv_account_SOURCES += \
	dal/sqlitedataprovider.h \
	dal/sqlitedataprovider.cpp
endif

if BUILD_SQUIRREL
tmwserv_game_SOURCES += \
	script.h \
	script.cpp \
	script-squirrel.h \
	script-squirrel.cpp
endif

if BUILD_RUBY
bindings_wrap.cpp: bindings.i
	$(SWIG) $(SWIGFLAGS) -o bindings_wrap.cpp bindings.i
tmwserv_game_SOURCES += \
	bindings_wrap.cpp
endif
