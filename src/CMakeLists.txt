FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(PhysFS REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

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

IF (POLICY CMP0015)
  CMAKE_POLICY(SET CMP0015 OLD)
ENDIF()
INCLUDE_DIRECTORIES("../libs/enet/include")
LINK_DIRECTORIES("../libs/enet")
SET(INTERNAL_LIBRARIES enet)

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
    SET(EXTRA_LIBRARIES ws2_32 winmm)
    # GDI APIs Rectangle clashes with tmwserv classes
    SET(FLAGS "${FLAGS} -DNOGDI")
ELSEIF (CMAKE_SYSTEM_NAME STREQUAL SunOS)
    # explicit linking to libintl is required on Solaris
    SET(EXTRA_LIBRARIES intl)
ENDIF()

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PHYSFS_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    )

# Fix some stuff that gets not hidden by mainline modules
MARK_AS_ADVANCED(PHYSFS_INCLUDE_DIR)
MARK_AS_ADVANCED(PHYSFS_LIBRARY)

SET(SRCS
    common/configuration.h
    common/configuration.cpp
    common/defines.h
    common/inventorydata.h
    common/manaserv_protocol.h
    common/resourcemanager.h
    common/resourcemanager.cpp
    net/bandwidth.h
    net/bandwidth.cpp
    net/connection.h
    net/connection.cpp
    net/connectionhandler.h
    net/connectionhandler.cpp
    net/messagein.h
    net/messagein.cpp
    net/messageout.h
    net/messageout.cpp
    net/netcomputer.h
    net/netcomputer.cpp
    serialize/characterdata.h
    utils/logger.h
    utils/logger.cpp
    utils/point.h
    utils/processorutils.h
    utils/processorutils.cpp
    utils/string.h
    utils/string.cpp
    utils/stringfilter.h
    utils/stringfilter.cpp
    utils/timer.h
    utils/timer.cpp
    utils/tokencollector.h
    utils/tokencollector.cpp
    utils/tokendispenser.h
    utils/tokendispenser.cpp
    utils/xml.h
    utils/xml.cpp
    )

SET(SRCS_MANASERVACCOUNT
    account-server/main-account.cpp
    account-server/account.h
    account-server/account.cpp
    account-server/accountclient.h
    account-server/accountclient.cpp
    account-server/accounthandler.h
    account-server/accounthandler.cpp
    account-server/character.h
    account-server/character.cpp
    account-server/flooritem.h
    account-server/serverhandler.h
    account-server/serverhandler.cpp
    account-server/storage.h
    account-server/storage.cpp
    chat-server/chathandler.h
    chat-server/chathandler.cpp
    chat-server/chatclient.h
    chat-server/chatchannel.h
    chat-server/chatchannel.cpp
    chat-server/chatchannelmanager.h
    chat-server/chatchannelmanager.cpp
    chat-server/guild.h
    chat-server/guild.cpp
    chat-server/guildhandler.cpp
    chat-server/guildmanager.h
    chat-server/guildmanager.cpp
    chat-server/party.cpp
    chat-server/party.h
    chat-server/partyhandler.cpp
    chat-server/post.cpp
    chat-server/post.h
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
    utils/throwerror.h
    utils/time.h
    )

SET(SRCS_MANASERVGAME
    game-server/main-game.cpp
    common/permissionmanager.h
    common/permissionmanager.cpp
    game-server/accountconnection.h
    game-server/accountconnection.cpp
    game-server/actor.h
    game-server/actor.cpp
    game-server/attribute.h
    game-server/attribute.cpp
    game-server/attributemanager.h
    game-server/attributemanager.cpp
    game-server/autoattack.h
    game-server/autoattack.cpp
    game-server/being.h
    game-server/being.cpp
    game-server/buysell.h
    game-server/buysell.cpp
    game-server/character.h
    game-server/character.cpp
    game-server/collisiondetection.h
    game-server/collisiondetection.cpp
    game-server/command.cpp
    game-server/commandhandler.cpp
    game-server/commandhandler.h
    game-server/effect.h
    game-server/effect.cpp
    game-server/eventlistener.h
    game-server/gamehandler.h
    game-server/gamehandler.cpp
    game-server/inventory.h
    game-server/inventory.cpp
    game-server/item.h
    game-server/item.cpp
    game-server/itemmanager.h
    game-server/itemmanager.cpp
    game-server/map.h
    game-server/map.cpp
    game-server/mapcomposite.h
    game-server/mapcomposite.cpp
    game-server/mapmanager.h
    game-server/mapmanager.cpp
    game-server/mapreader.h
    game-server/mapreader.cpp
    game-server/monster.h
    game-server/monster.cpp
    game-server/monstermanager.h
    game-server/monstermanager.cpp
    game-server/npc.h
    game-server/npc.cpp
    game-server/postman.h
    game-server/quest.h
    game-server/quest.cpp
    game-server/skillmanager.h
    game-server/skillmanager.cpp
    game-server/spawnarea.h
    game-server/spawnarea.cpp
    game-server/state.h
    game-server/state.cpp
    game-server/statuseffect.h
    game-server/statuseffect.cpp
    game-server/statusmanager.h
    game-server/statusmanager.cpp
    game-server/thing.h
    game-server/thing.cpp
    game-server/trade.h
    game-server/trade.cpp
    game-server/trigger.h
    game-server/trigger.cpp
    scripting/script.h
    scripting/script.cpp
    utils/base64.h
    utils/base64.cpp
    utils/mathutils.h
    utils/mathutils.cpp
    utils/speedconv.h
    utils/speedconv.cpp
    utils/zlib.h
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
    scripting/luascript.h
    scripting/luautil.cpp
    scripting/luautil.h)
ENDIF()


SET (PROGRAMS manaserv-account manaserv-game)

ADD_EXECUTABLE(manaserv-game WIN32 ${SRCS} ${SRCS_MANASERVGAME})
ADD_EXECUTABLE(manaserv-account WIN32 ${SRCS} ${SRCS_MANASERVACCOUNT})

FOREACH(program ${PROGRAMS})
    TARGET_LINK_LIBRARIES(${program} ${INTERNAL_LIBRARIES}
        ${PHYSFS_LIBRARY}
        ${LIBXML2_LIBRARIES}
        ${ZLIB_LIBRARIES}
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
