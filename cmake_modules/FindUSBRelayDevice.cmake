# - Try to find the USB-RELAY-DEVICE library, libusb_relay_device.so
# Once done this defines
#
#  USB_RELAY_DEVICE_FOUND - system has libusb_interface_shared.so
#  USB_RELAY_DEVICE_INCLUDE_DIR - the libusb_interface_shared include directory
#  USB_RELAY_DEVICE_LIBRARIES - Link these to use libusb_interface_shared

# Copyright (c) 2015, Magnus W. Eriksen
#
# Based on FindNova
# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING.BSD file.

if (USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)

  	# in cache already
  	set(USB_RELAY_DEVICE_FOUND TRUE)
    message(STATUS "Found USB_RELAY_DEVICE: ${USB_RELAY_DEVICE_LIBRARIES}")

else (USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)

  	find_path(USB_RELAY_DEVICE_INCLUDE_DIR usb_relay_device.h
      	HINTS ${/usr/local/include}
    )

	find_library(USB_RELAY_DEVICE_LIBRARIES NAMES libusb_relay_device.so
    	HINTS ${/usr/local/lib}
    )

    set(CMAKE_REQUIRED_INCLUDES ${USB_RELAY_DEVICE_INCLUDE_DIR})
    set(CMAKE_REQUIRED_LIBRARIES ${USB_RELAY_DEVICE_LIBRARIES})

    if(USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)
        set(USB_RELAY_DEVICE_FOUND TRUE)
    else(USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)
        set(USB_RELAY_DEVICE_FOUND FALSE)
    endif (USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)

    if(USB_RELAY_DEVICE_FOUND)
        message(STATUS "Found USB_RELAY_DEVICE: ${USB_RELAY_DEVICE_LIBRARIES}")
    else(USB_RELAY_DEVICE_FOUND)
        message(FATAL_ERROR "USB_RELAY_DEVICE: Not Found!
        The library must be compiled from source,
        see indi-usbrelay2-roof/INSTALL for instructions")
    endif(USB_RELAY_DEVICE_FOUND)

	mark_as_advanced(USB_RELAY_DEVICE_INCLUDE_DIR USB_RELAY_DEVICE_LIBRARIES)

endif (USB_RELAY_DEVICE_INCLUDE_DIR AND USB_RELAY_DEVICE_LIBRARIES)
