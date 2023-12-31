# configuration for target
MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# create a list of device drivers
SET(ENABLED_DRIVERS
		delay
		bootloader_api
		stack_struct
		i2cdrv
		temperature_tmp116
		radio_template
		radio_rfm9x
		radio_efr32
		reset_info
		serial
		wg_mac
		wg_mac_ncp
        imu_fxos
		battery_monitor
		simeeprom
		gpcrc
		mfg_hw_test


		# Some legacy drivers
		# -------------------
		#
		# pa_sky66112
		# eeprom_cat24c16
		# dynamic_callbacks
		# ncp_spi
		# simgbpr
		# lorawan
		# lora_phy
		# mbedtls
		)

INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})

# include dependencies
FOREACH(MODULE ${ENABLED_DRIVERS})
	SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY
			${TARGET_SPECIFIC_INCLUDE_DIRECTORY}
			${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}
			)

	# update include list
	INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})
ENDFOREACH(MODULE)

# build modules
FOREACH(MODULE ${ENABLED_DRIVERS})
	# compile those libraries
	ADD_SUBDIRECTORY("${MODULE}")
ENDFOREACH(MODULE)

# Collect all source file
FILE (GLOB SRC *.c *.s)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})

# add dependency between libraries
ADD_DEPENDENCIES(${LIB_NAME} ${ENABLED_DRIVERS})

# link libraries with top level library
TARGET_LINK_LIBRARIES(${LIB_NAME} ${ENABLED_DRIVERS})

# forward include header files
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY ${TARGET_SPECIFIC_INCLUDE_DIRECTORY} PARENT_SCOPE)
