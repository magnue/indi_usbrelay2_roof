cmake_minimum_required(VERSION 2.6)
PROJECT(indi_usbrelay2_roof CXX)

cmake_policy(SET CMP0003 NEW)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")

##################  Paths  ###################################

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")


IF(APPLE)
set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
ENDIF(APPLE)

##################  Includes  ################################

include (MacroLogFeature)
include (MacroBoolTo01)

FIND_PACKAGE(INDIDriver REQUIRED)
FIND_PACKAGE(Nova REQUIRED)
FIND_PACKAGE(USBRelayDevice REQUIRED)

macro_bool_to_01(INDI_DRIVER_LIBRARIES HAVE_INDIDRIVER_LIB)
macro_bool_to_01(NOVA_FOUND HAVE_NOVA_H)
macro_log_feature(NOVA_FOUND "libnova" "A general purpose, double precision, Celestial Mechanics, Astrometry and Astrodynamics library" "http://libnova.sourceforge.net" FALSE "0.12.1" "Provides INDI with astrodynamics library.")
macro_bool_to_01(USB_RELAY_DEVICE_FOUND HAVE_USB_RELAY_DEVICE_H)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})
include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_SOURCE_DIR}/usbrelay2_roof)

################ USB RELAY2 ##############################

set(usbrelay2_SRCS
        ${CMAKE_SOURCE_DIR}/usbrelay2_roof/usbrelay2_roof.cpp
        ${CMAKE_SOURCE_DIR}/usbrelay2_roof/usb_interface.cpp
   )

add_executable(indi_usbrelay2_roof ${usbrelay2_SRCS})
target_link_libraries(indi_usbrelay2_roof ${INDI_DRIVER_LIBRARIES} ${USB_RELAY_DEVICE_LIBRARIES})
install(TARGETS indi_usbrelay2_roof RUNTIME DESTINATION bin )
install(FILES ${CMAKE_SOURCE_DIR}/usbrelay2_roof/99-usbrelay2.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
install( FILES ${CMAKE_SOURCE_DIR}/usbrelay2_roof/indi_usbrelay2_roof.xml DESTINATION ${DATA_INSTALL_DIR})

###########################################################
