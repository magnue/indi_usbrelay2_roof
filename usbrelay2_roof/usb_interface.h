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

    const unsigned int bits[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    int Init();
    bool TestConnect(char* devSerial);
    vector<const char*> GetDevices();
    vector<unsigned int> GetChannelsForDevice(char* devSerial, int numChannels);
    int OpenClose(char* serial, bool open);
    int OpenCloseChannel(char* serial, bool open, int channel);

    ~USBInterface();

    private:
    pusb_relay_device_info_t dev_enum;

};

#endif /* USB_INTERFACE_H */
