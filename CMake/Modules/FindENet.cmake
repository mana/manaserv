# - Try to find enet
# Once done this will define
#
#  ENET_FOUND - system has enet
#  ENET_INCLUDE_DIR - the enet include directory
#  ENET_LIBRARIES - the libraries needed to use enet
#  ENET_DEFINITIONS - Compiler switches required for using enet

IF (ENet_INCLUDE_DIR AND ENet_LIBRARY)
   SET(ENet_FIND_QUIETLY TRUE)
ENDIF (ENet_INCLUDE_DIR AND ENet_LIBRARY)

# for Windows we rely on the environement variables
# %INCLUDE% and %LIB%; FIND_LIBRARY checks %LIB%
# automatically on Windows
IF(WIN32)
    FIND_PATH(ENet_INCLUDE_DIR enet/enet.h
        $ENV{INCLUDE}
    )
    FIND_LIBRARY(ENet_LIBRARY
        NAMES enet
    )
ELSE()
    FIND_PATH(ENet_INCLUDE_DIR enet/enet.h
        /usr/include
        /usr/local/include
    )
    FIND_LIBRARY(ENet_LIBRARY
        NAMES enet
        PATHS /usr/lib /usr/local/lib
    )
ENDIF()

IF (ENet_INCLUDE_DIR AND ENet_LIBRARY)
    SET(ENET_FOUND TRUE)
    SET(ENET_INCLUDE_DIR ${ENet_INCLUDE_DIR})
    SET(ENET_LIBRARIES ${ENet_LIBRARY})
ELSE ()
    SET(ENET_FOUND FALSE)
ENDIF ()

IF (ENET_FOUND)
    IF (NOT ENet_FIND_QUIETLY)
        MESSAGE(STATUS "Found enet: ${ENet_LIBRARY}")
    ENDIF (NOT ENet_FIND_QUIETLY)
ELSE (ENET_FOUND)
    IF (ENet_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could NOT find enet")
    ENDIF (ENet_FIND_REQUIRED)
ENDIF (ENET_FOUND)

MARK_AS_ADVANCED(ENet_INCLUDE_DIR ENet_LIBRARY)
