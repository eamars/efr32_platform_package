MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# build submodule first
INCLUDE_DIRECTORIES(crypto)
ADD_SUBDIRECTORY(crypto)

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB SRC *.c)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})

# add dependency and build order
ADD_DEPENDENCIES(${LIB_NAME} radio_rfm9x crypto)
TARGET_LINK_LIBRARIES(${LIB_NAME} radio_rfm9x crypto)
