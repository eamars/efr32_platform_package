# configuration for target
MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB EFR32_SRC *.c)


# define C flags
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -W -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -std=gnu11 -Wall -Wextra -Wno-unused -Wno-unused-parameter -fmessage-length=0 -ffunction-sections -fdata-sections")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -W -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -std=gnu11 -Wall -Wextra -Wno-unused -Wno-unused-parameter -fmessage-length=0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -gdwarf-2 -mcpu=cortex-m4 -mthumb -c -x assembler-with-cpp")

# DEBUG LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -mthumb -Wl,--no-wchar-size-warning,--no-enum-size-warning -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16")

SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY
		"${TARGET_SPECIFIC_INCLUDE_DIRECTORY}"

		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/common"
		"${CMAKE_CURRENT_SOURCE_DIR}/drivers"
		"${CMAKE_CURRENT_SOURCE_DIR}/freertos/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/freertos_trace/Include"
		)

# include essential driver headers
INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})
INCLUDE_DIRECTORIES(${BOARD_SPECIFIC_INCLUDE_DIRECTORY})

# build drivers
ADD_SUBDIRECTORY(freertos)
ADD_SUBDIRECTORY(freertos_trace)
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(drivers)


# build sam4s library
ADD_LIBRARY(${LIB_NAME} ${EFR32_SRC})

ADD_DEPENDENCIES(${LIB_NAME}
		freertos
		freertos_trace
		common
		drivers
		)

TARGET_LINK_LIBRARIES(${LIB_NAME}
		freertos
		freertos_trace
		common
		drivers
		)

# set those variables visible to parents
SET(CMAKE_ASM_FLAGS_DEBUG ${CMAKE_ASM_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG} PARENT_SCOPE)

SET(CMAKE_ASM_FLAGS_RELEASE ${CMAKE_ASM_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE} PARENT_SCOPE)

# forward include header files
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY ${TARGET_SPECIFIC_INCLUDE_DIRECTORY} PARENT_SCOPE)
