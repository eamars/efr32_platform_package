MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE(GLOB SRC *.c)

# collect all pre-built library
FILE(GLOB STATIC_OBJECT static/*.o)

# include necessary directory
INCLUDE_DIRECTORIES(
		${TARGET_ROOT_DIRECTORY}/protocol/flex_2.0/connect/plugins
		${TARGET_ROOT_DIRECTORY}/platform/base/hal/plugin/sim-eeprom
		${TARGET_ROOT_DIRECTORY}/protocol/flex_2.0/connect/plugins/stack/include
		${TARGET_ROOT_DIRECTORY}/protocol/flex_2.0/connect/plugins/stack
)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})
TARGET_LINK_LIBRARIES(${LIB_NAME} ${STATIC_OBJECT})
