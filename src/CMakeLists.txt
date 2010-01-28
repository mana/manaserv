FIND_PACKAGE(ENet REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(PhysFS REQUIRED)

IF (CMAKE_COMPILER_IS_GNUCXX)
    # Help getting compilation warnings
    SET(CMAKE_CXX_FLAGS "-Wall")
    IF (WIN32)
        # This includes enough debug information to get something useful
        # from Dr. Mingw while keeping binary size down. Almost useless
        # with gdb, though.
        SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -ggdb0 -gstabs2")
    ENDIF()
ENDIF()

SET(FLAGS "-DPACKAGE_VERSION=\\\"${VERSION}\\\"")
SET(FLAGS "${FLAGS} -DPKG_DATADIR=\\\"${PKG_DATADIR}/\\\"")
SET(FLAGS "${FLAGS} -DLOCALEDIR=\\\"${LOCALEDIR}/\\\"")

IF (CMAKE_BUILD_TYPE)
    STRING(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    IF((CMAKE_BUILD_TYPE_TOLOWER MATCHES debug) OR
       (CMAKE_BUILD_TYPE_TOLOWER MATCHES relwithdebinfo))
        SET(FLAGS "${FLAGS} -DDEBUG")
    ENDIF()
ENDIF()

IF (WIN32)
    SET(EXTRA_LIBRARIES ws2_32 winmm)
    FIND_PACKAGE(LibIntl REQUIRED)
ELSEIF (CMAKE_SYSTEM_NAME STREQUAL SunOS)
    # explicit linking to libintl is required on Solaris
    SET(EXTRA_LIBRARIES intl)
ENDIF()

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENET_INCLUDE_DIR}
    ${PHYSFS_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    )

# Fix some stuff that gets not hidden by mainline modules
MARK_AS_ADVANCED(PHYSFS_INCLUDE_DIR)
MARK_AS_ADVANCED(PHYSFS_LIBRARY)

SET(SRCS_MANASERVACCOUNT
    account-server/main-account.cpp
    defines.h
    protocol.h
    point.h
    account-server/account.hpp
    account-server/account.cpp
    account-server/accountclient.hpp
    account-server/accountclient.cpp
    account-server/accounthandler.hpp
    account-server/accounthandler.cpp
    account-server/character.hpp
    account-server/character.cpp
    account-server/serverhandler.hpp
    account-server/serverhandler.cpp
    account-server/storage.hpp
    account-server/storage.cpp
    chat-server/chathandler.hpp
    chat-server/chathandler.cpp
    chat-server/chatclient.hpp
    chat-server/chatchannel.hpp
    chat-server/chatchannel.cpp
    chat-server/chatchannelmanager.hpp
    chat-server/chatchannelmanager.cpp
    chat-server/guild.hpp
    chat-server/guild.cpp
    chat-server/guildhandler.cpp
    chat-server/guildmanager.hpp
    chat-server/guildmanager.cpp
    chat-server/party.cpp
    chat-server/party.hpp
    chat-server/partyhandler.cpp
    chat-server/post.cpp
    chat-server/post.hpp
    common/configuration.hpp
    common/configuration.cpp
    common/inventorydata.hpp
    dal/dalexcept.h
    dal/dataprovider.h
    dal/dataprovider.cpp
    dal/dataproviderfactory.h
    dal/dataproviderfactory.cpp
    dal/recordset.h
    dal/recordset.cpp
    net/bandwidth.hpp
    net/bandwidth.cpp
    net/connectionhandler.hpp
    net/connectionhandler.cpp
    net/messagein.hpp
    net/messagein.cpp
    net/messageout.hpp
    net/messageout.cpp
    net/netcomputer.hpp
    net/netcomputer.cpp
    serialize/characterdata.hpp
    utils/functors.h
    utils/encryption.h
    utils/encryption.cpp
    utils/logger.h
    utils/logger.cpp
    utils/processorutils.hpp
    utils/processorutils.cpp
    utils/sha256.h
    utils/sha256.cpp
    utils/stringfilter.h
    utils/stringfilter.cpp
    utils/timer.cpp
    utils/tokencollector.hpp
    utils/tokencollector.cpp
    utils/tokendispenser.hpp
    utils/tokendispenser.cpp
    utils/xml.hpp
    utils/xml.cpp
    utils/string.cpp
    )

SET(SRCS_MANASERVGAME
    game-server/main-game.cpp
    defines.h
    protocol.h
    point.h
    common/configuration.hpp
    common/configuration.cpp
    common/inventorydata.hpp
    common/permissionmanager.hpp
    common/permissionmanager.cpp
    game-server/accountconnection.hpp
    game-server/accountconnection.cpp
    game-server/actor.hpp
    game-server/actor.cpp
    game-server/being.hpp
    game-server/being.cpp
    game-server/buysell.hpp
    game-server/buysell.cpp
    game-server/character.hpp
    game-server/character.cpp
    game-server/collisiondetection.hpp
    game-server/collisiondetection.cpp
    game-server/command.cpp
    game-server/commandhandler.cpp
    game-server/commandhandler.hpp
    game-server/effect.hpp
    game-server/effect.cpp
    game-server/eventlistener.hpp
    game-server/gamehandler.hpp
    game-server/gamehandler.cpp
    game-server/inventory.hpp
    game-server/inventory.cpp
    game-server/item.hpp
    game-server/item.cpp
    game-server/itemmanager.hpp
    game-server/itemmanager.cpp
    game-server/map.hpp
    game-server/map.cpp
    game-server/mapcomposite.hpp
    game-server/mapcomposite.cpp
    game-server/mapmanager.hpp
    game-server/mapmanager.cpp
    game-server/mapreader.hpp
    game-server/mapreader.cpp
    game-server/monster.hpp
    game-server/monster.cpp
    game-server/monstermanager.hpp
    game-server/monstermanager.cpp
    game-server/npc.hpp
    game-server/npc.cpp
    game-server/postman.hpp
    game-server/quest.hpp
    game-server/quest.cpp
    game-server/resourcemanager.hpp
    game-server/resourcemanager.cpp
    game-server/skillmanager.hpp
    game-server/skillmanager.cpp
    game-server/spawnarea.hpp
    game-server/spawnarea.cpp
    game-server/state.hpp
    game-server/state.cpp
    game-server/statuseffect.hpp
    game-server/statuseffect.cpp
    game-server/statusmanager.hpp
    game-server/statusmanager.cpp
    game-server/thing.hpp
    game-server/thing.cpp
    game-server/trade.hpp
    game-server/trade.cpp
    game-server/trigger.hpp
    game-server/trigger.cpp
    net/bandwidth.hpp
    net/bandwidth.cpp
    net/connection.hpp
    net/connection.cpp
    net/connectionhandler.hpp
    net/connectionhandler.cpp
    net/messagein.hpp
    net/messagein.cpp
    net/messageout.hpp
    net/messageout.cpp
    net/netcomputer.hpp
    net/netcomputer.cpp
    scripting/script.hpp
    scripting/script.cpp
    serialize/characterdata.hpp
    utils/base64.h
    utils/base64.cpp
    utils/mathutils.h
    utils/mathutils.cpp
    utils/logger.h
    utils/logger.cpp
    utils/processorutils.hpp
    utils/processorutils.cpp
    utils/stringfilter.h
    utils/stringfilter.cpp
    utils/timer.h
    utils/timer.cpp
    utils/trim.hpp
    utils/tokencollector.hpp
    utils/tokencollector.cpp
    utils/tokendispenser.hpp
    utils/tokendispenser.cpp
    utils/xml.hpp
    utils/xml.cpp
    utils/zlib.hpp
    utils/zlib.cpp
    utils/string.cpp
    )


SET (PROGRAMS manaserv-account manaserv-game)

ADD_EXECUTABLE(manaserv-account WIN32 ${SRCS} ${SRCS_MANASERVGAME})
ADD_EXECUTABLE(manaserv-game WIN32 ${SRCS} ${SRCS_MANASERVACCOUNT})

FOREACH(program ${PROGRAMS})
    TARGET_LINK_LIBRARIES(${program} ${ENET_LIBRARIES}
        ${PHYSFS_LIBRARY}
        ${LIBXML2_LIBRARIES}
        ${EXTRA_LIBRARIES})
    INSTALL(TARGETS ${program} RUNTIME DESTINATION ${PKG_BINDIR})
ENDFOREACH(program)

IF (CMAKE_SYSTEM_NAME STREQUAL SunOS)
    # we expect the SMCgtxt package to be present on Solaris;
    # the Solaris gettext is not API-compatible to GNU gettext
    SET_TARGET_PROPERTIES(manaserv-account PROPERTIES LINK_FLAGS "-L/usr/local/lib")
    SET_TARGET_PROPERTIES(manaserv-game PROPERTIES LINK_FLAGS "-L/usr/local/lib")
ENDIF()
