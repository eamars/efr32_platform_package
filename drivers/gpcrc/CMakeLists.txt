MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB SRC *.c)

# add additional source
SET(SRC ${SRC} ${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_gpcrc.c)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})

# add dependency and build order
#ADD_DEPENDENCIES(${LIB_NAME} emlib)
#TARGET_LINK_LIBRARIES(${LIB_NAME} emlib)
