MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE(GLOB SRC *.c)

# append additional src file
SET(SRC
		${SRC}
		${TARGET_ROOT_DIRECTORY}/util/third_party/mbedtls/library/aes.c
		${TARGET_ROOT_DIRECTORY}/util/third_party/mbedtls/library/cmac.c
)

# include necessary directory
INCLUDE_DIRECTORIES(
		${TARGET_ROOT_DIRECTORY}/util/third_party/mbedtls
		${TARGET_ROOT_DIRECTORY}/util/third_party/mbedtls/include
		${TARGET_ROOT_DIRECTORY}/util/third_party/mbedtls/sl_crypto/include
)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})
# TARGET_LINK_LIBRARIES(${LIB_NAME})
