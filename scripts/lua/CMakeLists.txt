SET (FILES
    libmana-constants.lua
    libmana.lua
    npclib.lua
    )

INSTALL(FILES ${FILES} DESTINATION ${PKG_DATADIR}/scripts/lua/)
