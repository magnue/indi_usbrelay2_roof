/*******************************************************************************
 USBInterface 
 intermediate class for communication between USBRelay2 Roof and the usb-relay-hid API
 Copyright (c) 2015-2020 Magnus W. Eriksen

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
#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

//Include the API
#include "usb_relay_device.h"

#include <vector>

using namespace std;

class USBInterface
{
    public:

    USBInterface();

    const unsigned int bits[8] = {1,2,4,8,16,32,64,128};
    int Init();
    bool TestConnect(char* devSerial);
    vector<const char*> GetDevices();
    int GetNumberOfChannelsForDevice(char* devSerial);
    vector<bool> GetChannelsForDevice(char* devText);
    int OpenClose(char* devText, bool open);
    int OpenCloseChannel(char* devText, bool open);

    ~USBInterface();

    private:
    char* GetSerialFromDevice(char* devText);
    int GetChannelFromDevice(char* devText);
    pusb_relay_device_info_t dev_enum;

};

#endif /* USB_INTERFACE_H */
