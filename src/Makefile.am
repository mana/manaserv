# $Id$

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = tmwserv-account tmwserv-game

tmwserv_account_SOURCES = \
	account-server/main-account.cpp \
	abstractcharacterdata.hpp \
	abstractcharacterdata.cpp \
	configuration.h \
	configuration.cpp \
	defines.h \
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
	account-server/characterdata.hpp \
	account-server/characterdata.cpp \
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
	utils/countedptr.h \
	utils/functors.h \
	utils/logger.h \
	utils/logger.cpp \
	utils/processorutils.hpp \
	utils/processorutils.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/tokencollector.hpp \
	utils/tokendispenser.hpp \
	utils/tokendispenser.cpp

tmwserv_game_SOURCES = \
	game-server/main-game.cpp \
	abstractcharacterdata.hpp \
	abstractcharacterdata.cpp \
	configuration.h \
	configuration.cpp \
	defines.h \
	point.h \
	resourcemanager.h \
	resourcemanager.cpp \
	skill.h \
	skill.cpp \
	game-server/accountconnection.hpp \
	game-server/accountconnection.cpp \
	game-server/being.hpp \
	game-server/being.cpp \
	game-server/character.hpp \
	game-server/character.cpp \
	game-server/collisiondetection.hpp \
	game-server/collisiondetection.cpp \
	game-server/gamehandler.hpp \
	game-server/gamehandler.cpp \
	game-server/inventory.hpp \
	game-server/inventory.cpp \
	game-server/item.hpp \
	game-server/item.cpp \
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
	game-server/monster.hpp \
	game-server/monster.cpp \
	game-server/movingobject.hpp \
	game-server/movingobject.cpp \
	game-server/object.hpp \
	game-server/state.hpp \
	game-server/state.cpp \
	game-server/testing.cpp \
	game-server/thing.hpp \
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
	utils/mathutils.h \
	utils/mathutils.cpp \
	utils/logger.h \
	utils/logger.cpp \
	utils/processorutils.hpp \
	utils/processorutils.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/timer.h \
	utils/timer.cpp \
	utils/tokencollector.hpp \
	utils/tokendispenser.hpp \
	utils/tokendispenser.cpp \
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
