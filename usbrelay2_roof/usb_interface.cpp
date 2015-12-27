/*******************************************************************************
 USBInterface 
 intermediate class for communication between USBRelay2 Roof and the usb-relay-hid API
 Copyright (c) 2015 Magnus W. Eriksen

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file LICENSE.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include "usb_interface.h"

#include <string.h>
#include <string>

using namespace std;

USBInterface::USBInterface()
{
}

int USBInterface::Init()
{
    dev_enum = nullptr;
    return usb_relay_init(); // returns -1 if fail, 0 on success
}

bool USBInterface::TestConnect(char* devSerial)
{
    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    if (devHandle == 0)
        return false;
    usb_relay_device_close(devHandle);
    return true;
}

vector<const char*> USBInterface::GetDevices()
{
    if (dev_enum)
    {
        usb_relay_device_free_enumerate(dev_enum);
	    dev_enum = nullptr;
    }
    dev_enum = usb_relay_device_enumerate();
    
    vector<const char*> serialNumber;
    for (pusb_relay_device_info_t i = dev_enum; i; i = i->next)
        serialNumber.push_back(i->serial_number);

    return serialNumber; // serialNumber, vector of device ID. Might be empty
}

vector<unsigned int> USBInterface::GetChannelsForDevice(char* devSerial, int numChannels)
{
    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    unsigned int status;
    vector<unsigned int> channelStatus;

    ret = usb_relay_device_get_status(devHandle, &status);
    if(ret != 0)
        return channelStatus;

    for (int i = 0; i < numChannels; ++i)
    {
        channelStatus.push_back(status & bits[i]);
    }

    usb_relay_device_close(devHandle);
    
    return channelStatus; // channelStatus, vector of boolean channel status. Might be empty
}

int USBInterface::OpenClose(char* devSerial, bool open)
{
    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    if (open)
        ret = usb_relay_device_open_all_relay_channel(devHandle);
    else
        ret = usb_relay_device_close_all_relay_channel(devHandle);

    usb_relay_device_close(devHandle);

    return ret; // ret == 0 on success, all else fail
}

int USBInterface::OpenCloseChannel(char* devSerial, bool open, int channel)
{
    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    if (open)
        ret = usb_relay_device_open_one_relay_channel(devHandle, channel);
    else
        ret = usb_relay_device_close_one_relay_channel(devHandle, channel);

    usb_relay_device_close(devHandle);

    return ret; // ret == 0 on success, all else fail
}

USBInterface::~USBInterface()
{
    /* Frees static data associated with USB Relay library.
    0 on success, -1 fail */
    usb_relay_exit();
}
