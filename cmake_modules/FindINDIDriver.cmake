# - Try to find INDI
# Once done this will define
#
#  INDI_DRIVER_FOUND - system has INDI-DRIVER
#  INDI_INCLUDE_DIR - the INDI include directory
#  INDI_DRIVER_LIBRARIES - Link to these to build INDI drivers with indibase support

# Copyright (c) 2015, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Copyright (c) 2012, Pino Toscano <pino@kde.org>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING.BSD file.

if (INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES )

  # in cache already
  set(INDI_DRIVER_FOUND TRUE)
  message(STATUS "Found INDI-DRIVERS: ${INDI_DRIVER_LIBRARIES}")


else (INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES)


    find_path(INDI_INCLUDE_DIR indidevapi.h
      PATH_SUFFIXES libindi
      HINTS ${PC_INDI_INCLUDE_DIRS}
    )

    find_library(INDI_DRIVER_LIBRARIES NAMES indidriver
      HINTS ${PC_INDI_LIBRARY_DIRS}
    )
  
    if(INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES)
        set(INDI_DRIVER_FOUND TRUE)
    else(INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES)
        set(INDI_DRIVER_FOUND FALSE)
    endif(INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES)
    
    if(INDI_DRIVER_FOUND)
        message(STATUS "Found INDI-DRIVERS: ${INDI_DRIVER_LIBRARIES}")
    else(INDI_DRIVER_FOUND)
        message(FATAL_ERROR "INDI-DRIVERS not found!")
    endif(INDI_DRIVER_FOUND)

  mark_as_advanced(INDI_INCLUDE_DIR INDI_DRIVER_LIBRARIES)

endif (INDI_INCLUDE_DIR AND INDI_DRIVER_LIBRARIES)
