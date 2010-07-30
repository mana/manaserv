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

# enable rc-handling with mingw
# most likely this part can be kicked out with some later cmake version
IF (MINGW)
    FIND_PATH(MINGW_INCLUDE_DIR windows.h $ENV{INCLUDE})
    IF (MINGW_INCLUDE_DIR)
        MESSAGE(STATUS "Found mingw headers: ${MINGW_INCLUDE_DIR}")
        INCLUDE_DIRECTORIES(${MINGW_INCLUDE_DIR})
    ELSE()
        MESSAGE(FATAL_ERROR "Unable to find mingw headers. Required for windres")
    ENDIF()
    SET(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENDIF()

SET(FLAGS "-DPACKAGE_VERSION=\\\"${VERSION}\\\"")
SET(FLAGS "${FLAGS} -DPKG_DATADIR=\\\"${PKG_DATADIR}/\\\"")
SET(FLAGS "${FLAGS} -DLOCALEDIR=\\\"${LOCALEDIR}/\\\"")

# If the Sqlite option is enabled...
IF (WITH_SQLITE)
    FIND_PACKAGE(Sqlite3 REQUIRED)
    INCLUDE_DIRECTORIES(${SQLITE3_INCLUDE_DIR})
    SET(FLAGS "${FLAGS} -DSQLITE_SUPPORT")
    SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${SQLITE3_LIBRARIES})
ENDIF(WITH_SQLITE)

# If the MySQL option is enabled...
IF (WITH_MYSQL)
    FIND_PACKAGE(MySQL REQUIRED)
    INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})
    SET(FLAGS "${FLAGS} -DMYSQL_SUPPORT")
    SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${MYSQL_LIBRARIES})
ENDIF()

# If the postgreSQL option is enabled...
IF (WITH_POSTGRESQL)
    FIND_PACKAGE(PostgreSQL REQUIRED)
    INCLUDE_DIRECTORIES(${POSTGRESQL_INCLUDE_DIR})
    SET(FLAGS "${FLAGS} -DPOSTGRESQL_SUPPORT")
    SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${POSTGRESQL_LIBRARIES})
ENDIF()

# If the Lua scripting language support is enabled...
IF (ENABLE_LUA)
    FIND_PACKAGE(Lua51 REQUIRED)
    INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})
    SET(FLAGS "${FLAGS} -DBUILD_LUA")
    SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${LUA_LIBRARIES})
ENDIF()

IF (CMAKE_BUILD_TYPE)
    STRING(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    IF(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug OR
       CMAKE_BUILD_TYPE_TOLOWER MATCHES relwithdebinfo)
        SET(FLAGS "${FLAGS} -DDEBUG")
    ENDIF()
ENDIF()

IF (WIN32)
    # on the client zlib gets pulled in by LibPNG, for the server
    # we need to add it explicitly
    FIND_PACKAGE(ZLIB REQUIRED)
    SET(EXTRA_LIBRARIES ws2_32 winmm ${ZLIB_LIBRARIES})
    # GDI APIs Rectangle clashes with tmwserv classes
    SET(FLAGS "${FLAGS} -DNOGDI")
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

SET(SRCS
    defines.h
    protocol.h
    point.h
    common/configuration.hpp
    common/configuration.cpp
    common/inventorydata.hpp
    common/resourcemanager.hpp
    common/resourcemanager.cpp
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
    serialize/characterdata.hpp
    utils/logger.h
    utils/logger.cpp
    utils/processorutils.hpp
    utils/processorutils.cpp
    utils/string.hpp
    utils/string.cpp
    utils/stringfilter.h
    utils/stringfilter.cpp
    utils/timer.h
    utils/timer.cpp
    utils/tokencollector.hpp
    utils/tokencollector.cpp
    utils/tokendispenser.hpp
    utils/tokendispenser.cpp
    utils/xml.hpp
    utils/xml.cpp
    )

SET(SRCS_MANASERVACCOUNT
    account-server/main-account.cpp
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
    dal/dalexcept.h
    dal/dataprovider.h
    dal/dataprovider.cpp
    dal/dataproviderfactory.h
    dal/dataproviderfactory.cpp
    dal/recordset.h
    dal/recordset.cpp
    utils/functors.h
    utils/encryption.h
    utils/encryption.cpp
    utils/sha256.h
    utils/sha256.cpp
    )

SET(SRCS_MANASERVGAME
    game-server/main-game.cpp
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
    scripting/script.hpp
    scripting/script.cpp
    utils/base64.h
    utils/base64.cpp
    utils/mathutils.h
    utils/mathutils.cpp
    utils/trim.hpp
    utils/zlib.hpp
    utils/zlib.cpp
    )

IF (WIN32)
    SET(SRCS_MANASERVACCOUNT ${SRCS_MANASERVACCOUNT} manaserv-account.rc)
    SET(SRCS_MANASERVGAME ${SRCS_MANASERVGAME} manaserv-game.rc)
ENDIF ()

IF (WITH_MYSQL)
    SET(SRCS_MANASERVACCOUNT ${SRCS_MANASERVACCOUNT}
    dal/mysqldataprovider.h
    dal/mysqldataprovider.cpp)
ENDIF()

IF (WITH_POSTGRESQL)
    SET(SRCS_MANASERVACCOUNT ${SRCS_MANASERVACCOUNT}
    dal/pqdataprovider.h
    dal/pqdataprovider.cpp)
ENDIF()

IF (WITH_SQLITE)
    SET(SRCS_MANASERVACCOUNT ${SRCS_MANASERVACCOUNT}
    dal/sqlitedataprovider.h
    dal/sqlitedataprovider.cpp)
ENDIF()

IF (ENABLE_LUA)
    SET(SRCS_MANASERVGAME ${SRCS_MANASERVGAME}
    scripting/lua.cpp
    scripting/luascript.cpp
    scripting/luascript.hpp
    scripting/luautil.cpp
    scripting/luautil.hpp)
ENDIF()


SET (PROGRAMS manaserv-account manaserv-game)

ADD_EXECUTABLE(manaserv-game WIN32 ${SRCS} ${SRCS_MANASERVGAME})
ADD_EXECUTABLE(manaserv-account WIN32 ${SRCS} ${SRCS_MANASERVACCOUNT})

FOREACH(program ${PROGRAMS})
    TARGET_LINK_LIBRARIES(${program} ${ENET_LIBRARIES}
        ${PHYSFS_LIBRARY}
        ${LIBXML2_LIBRARIES}
        ${OPTIONAL_LIBRARIES}
        ${EXTRA_LIBRARIES})
    INSTALL(TARGETS ${program} RUNTIME DESTINATION ${PKG_BINDIR})
ENDFOREACH(program)

IF (CMAKE_SYSTEM_NAME STREQUAL SunOS)
    # we expect the SMCgtxt package to be present on Solaris;
    # the Solaris gettext is not API-compatible to GNU gettext
    SET_TARGET_PROPERTIES(manaserv-account PROPERTIES LINK_FLAGS "-L/usr/local/lib")
    SET_TARGET_PROPERTIES(manaserv-game PROPERTIES LINK_FLAGS "-L/usr/local/lib")
ENDIF()

SET_TARGET_PROPERTIES(manaserv-account PROPERTIES COMPILE_FLAGS "${FLAGS}")
SET_TARGET_PROPERTIES(manaserv-game PROPERTIES COMPILE_FLAGS "${FLAGS}")
