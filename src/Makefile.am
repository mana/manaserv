AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = manaserv-account manaserv-game

manaserv_account_CXXFLAGS = -Wall

manaserv_account_SOURCES = \
	account-server/main-account.cpp \
	defines.h \
	protocol.h \
	point.h \
	account-server/account.hpp \
	account-server/account.cpp \
	account-server/accountclient.hpp \
	account-server/accountclient.cpp \
	account-server/accounthandler.hpp \
	account-server/accounthandler.cpp \
	account-server/character.hpp \
	account-server/character.cpp \
	account-server/serverhandler.hpp \
	account-server/serverhandler.cpp \
	account-server/storage.hpp \
	account-server/storage.cpp \
	chat-server/chathandler.hpp \
	chat-server/chathandler.cpp \
	chat-server/chatclient.hpp \
	chat-server/chatchannel.hpp \
	chat-server/chatchannel.cpp \
	chat-server/chatchannelmanager.hpp \
	chat-server/chatchannelmanager.cpp \
	chat-server/guild.hpp \
	chat-server/guild.cpp \
	chat-server/guildhandler.cpp \
	chat-server/guildmanager.hpp \
	chat-server/guildmanager.cpp \
	chat-server/party.cpp \
	chat-server/party.hpp \
	chat-server/partyhandler.cpp \
	chat-server/post.cpp \
	chat-server/post.hpp \
	common/configuration.hpp \
	common/configuration.cpp \
	common/inventorydata.hpp \
	common/resourcemanager.hpp \
	common/resourcemanager.cpp \
	dal/dalexcept.h \
	dal/dataprovider.h \
	dal/dataprovider.cpp \
	dal/dataproviderfactory.h \
	dal/dataproviderfactory.cpp \
	dal/recordset.h \
	dal/recordset.cpp \
	net/bandwidth.hpp \
	net/bandwidth.cpp \
	net/connectionhandler.hpp \
	net/connectionhandler.cpp \
	net/messagein.hpp \
	net/messagein.cpp \
	net/messageout.hpp \
	net/messageout.cpp \
	net/netcomputer.hpp \
	net/netcomputer.cpp \
	serialize/characterdata.hpp \
	utils/functors.h \
	utils/encryption.h \
	utils/encryption.cpp \
	utils/logger.h \
	utils/logger.cpp \
	utils/processorutils.hpp \
	utils/processorutils.cpp \
	utils/sha256.h \
	utils/sha256.cpp \
	utils/string.hpp \
	utils/string.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/timer.cpp \
	utils/tokencollector.hpp \
	utils/tokencollector.cpp \
	utils/tokendispenser.hpp \
	utils/tokendispenser.cpp \
	utils/xml.hpp \
	utils/xml.cpp

manaserv_game_CXXFLAGS = -Wall

manaserv_game_SOURCES = \
	game-server/main-game.cpp \
	defines.h \
	protocol.h \
	point.h \
	common/configuration.hpp \
	common/configuration.cpp \
	common/inventorydata.hpp \
	common/permissionmanager.hpp \
	common/permissionmanager.cpp \
	common/resourcemanager.hpp \
	common/resourcemanager.cpp \
	game-server/accountconnection.hpp \
	game-server/accountconnection.cpp \
	game-server/actor.hpp \
	game-server/actor.cpp \
	game-server/attribute.hpp \
	game-server/attribute.cpp \
	game-server/attributemanager.hpp \
	game-server/attributemanager.cpp \
	game-server/autoattack.hpp \
	game-server/autoattack.cpp \
	game-server/being.hpp \
	game-server/being.cpp \
	game-server/buysell.hpp \
	game-server/buysell.cpp \
	game-server/character.hpp \
	game-server/character.cpp \
	game-server/collisiondetection.hpp \
	game-server/collisiondetection.cpp \
	game-server/command.cpp \
	game-server/commandhandler.cpp \
	game-server/commandhandler.hpp \
	game-server/effect.hpp \
	game-server/effect.cpp \
	game-server/eventlistener.hpp \
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
	game-server/monstermanager.hpp \
	game-server/monstermanager.cpp \
	game-server/npc.hpp \
	game-server/npc.cpp \
	game-server/postman.hpp \
	game-server/quest.hpp \
	game-server/quest.cpp \
	game-server/skillmanager.hpp \
	game-server/skillmanager.cpp \
	game-server/spawnarea.hpp \
	game-server/spawnarea.cpp \
	game-server/state.hpp \
	game-server/state.cpp \
	game-server/statuseffect.hpp \
	game-server/statuseffect.cpp \
	game-server/statusmanager.hpp \
	game-server/statusmanager.cpp \
	game-server/thing.hpp \
	game-server/thing.cpp \
	game-server/trade.hpp \
	game-server/trade.cpp \
	game-server/trigger.hpp \
	game-server/trigger.cpp \
	net/bandwidth.hpp \
	net/bandwidth.cpp \
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
	scripting/script.hpp \
	scripting/script.cpp \
	serialize/characterdata.hpp \
	utils/base64.h \
	utils/base64.cpp \
	utils/mathutils.h \
	utils/mathutils.cpp \
	utils/logger.h \
	utils/logger.cpp \
	utils/processorutils.hpp \
	utils/processorutils.cpp \
	utils/string.hpp \
	utils/string.cpp \
	utils/speedconv.hpp \
	utils/speedconv.cpp \
	utils/stringfilter.h \
	utils/stringfilter.cpp \
	utils/timer.h \
	utils/timer.cpp \
	utils/tokencollector.hpp \
	utils/tokencollector.cpp \
	utils/tokendispenser.hpp \
	utils/tokendispenser.cpp \
	utils/xml.hpp \
	utils/xml.cpp \
	utils/zlib.hpp \
	utils/zlib.cpp

if BUILD_MYSQL
manaserv_account_SOURCES += \
	dal/mysqldataprovider.h \
	dal/mysqldataprovider.cpp
endif

if BUILD_POSTGRESQL
manaserv_account_SOURCES += \
	dal/pqdataprovider.h \
	dal/pqdataprovider.cpp
endif

if BUILD_SQLITE
manaserv_account_SOURCES += \
	dal/sqlitedataprovider.h \
	dal/sqlitedataprovider.cpp
endif

if BUILD_LUA
manaserv_game_SOURCES += \
	scripting/lua.cpp \
	scripting/luascript.cpp \
	scripting/luascript.hpp \
	scripting/luautil.cpp \
	scripting/luautil.hpp
endif
