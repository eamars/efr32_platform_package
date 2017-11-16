MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB SRC *.c)

# Collect external files
SET(EXT_SRC
		)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC} ${EXT_SRC})

# add dependency and build order
#ADD_DEPENDENCIES(${LIB_NAME} emlib rail)
TARGET_LINK_LIBRARIES(${LIB_NAME}
		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/autogen/librail_release/librail_efr32xg12_gcc_release.a
		)
