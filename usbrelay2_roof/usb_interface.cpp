/*******************************************************************************
 USBInterface 
 intermediate class for communication between USBRelay2 Roof and the usb-relay-hid API
 Copyright (c) 2015 Magnus W. Eriksen

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License version 2.1 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 .
 You should have received a copy of the GNU Lesser General Public License
 along with this library; see the file LICENSE.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include "usb_interface.h"

#include <string.h>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <stdexcept>

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
    dev_enum = usb_relay_device_enumerate();
    
    vector<const char*> serialNumber;
    for (pusb_relay_device_info_t i = dev_enum; i != nullptr; i = i->next)
        serialNumber.push_back(i->serial_number);

    usb_relay_device_free_enumerate(dev_enum);
	dev_enum = nullptr;

    return serialNumber; // serialNumber, vector of device ID. Might be empty
}

int USBInterface::GetNumberOfChannelsForDevice(char* devSerial)
{
    dev_enum = usb_relay_device_enumerate();
    
    int type = 0;
    for (pusb_relay_device_info_t i = dev_enum; i != nullptr; i = i->next)
        if (strcmp(devSerial, i->serial_number)==0)
            return static_cast<int>(i->type);
    
    usb_relay_device_free_enumerate(dev_enum);
	dev_enum = nullptr;

    return type;
}

vector<bool> USBInterface::GetChannelsForDevice(char* devText)
{
    char* devSerial = GetSerialFromDevice(devText);
    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    unsigned int status;
    vector<bool> channelStatus;

    ret = usb_relay_device_get_status(devHandle, &status);
    if(ret != 0)
        return channelStatus;

    int numChannels = GetNumberOfChannelsForDevice(devSerial);
    for (int i = 0; i < numChannels; ++i)
    {
        channelStatus.push_back(status & bits[i]);
    }

    usb_relay_device_close(devHandle);
    
    return channelStatus; // channelStatus, vector of boolean channel status. Might be empty
}

int USBInterface::OpenClose(char* devText, bool open)
{
    char* devSerial = GetSerialFromDevice(devText);

    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    if (open)
        ret = usb_relay_device_open_all_relay_channel(devHandle);
    else
        ret = usb_relay_device_close_all_relay_channel(devHandle);

    usb_relay_device_close(devHandle);

    return ret; // ret == 0 on success, all else fail
}

int USBInterface::OpenCloseChannel(char* devText, bool open)
{
    char* devSerial = GetSerialFromDevice(devText);
    int channel = GetChannelFromDevice(devText);
    if (channel == 0)
        return 1;

    intptr_t devHandle = usb_relay_device_open_with_serial_number(devSerial, 5);
    int ret;
    if (open)
        ret = usb_relay_device_open_one_relay_channel(devHandle, channel);
    else
        ret = usb_relay_device_close_one_relay_channel(devHandle, channel);

    usb_relay_device_close(devHandle);

    return ret; // ret == 0 on success, all else fail
}

char* USBInterface::GetSerialFromDevice(char* devText)
{
    string input = devText;
    string devSerial;
    try {
        devSerial = input.substr(0,5);
    } catch (const std::out_of_range& ex) {
        return strdup("");
    }
    return strdup(devSerial.c_str());
}

int USBInterface::GetChannelFromDevice(char* devText)
{
    string input = devText;
    string channelString;
    try {
        channelString = input.substr(6,7);
    } catch (const std::out_of_range& ex) {
        return 0;
    }
    int channel;
    if (std::all_of(channelString.begin(), channelString.end(), ::isdigit))
        channel = atoi(channelString.c_str());
    else
        return 0;
    return channel;
}


USBInterface::~USBInterface()
{
    /* Frees static data associated with USB Relay library.
    0 on success, -1 fail */
    usb_relay_exit();
}
