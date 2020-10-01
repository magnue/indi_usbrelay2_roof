/*******************************************************************************
 USBRelay2 Roof # INDI driver for controlling usb-relay2 devices
 Copyright(c) 2015-2020 Magnus W. Eriksen

 based on: RollOff Roof by:
 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

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
#ifndef USBRelay2_H
#define USBRelay2_H

#include <libindi/indidome.h>
#include "usb_interface.h"

/*  Some headers we need */
#include <math.h>
#include <sys/time.h>
#include <vector>
using std::vector;

#define MAX_POWER_CHANNELS  10
#define CHANNEL_STATES      2
#define POWER_STATES        4

class USBRelay2 : public INDI::Dome
{

    public:
        USBRelay2();
        virtual ~USBRelay2();

        USBInterface usb;

        bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
        bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
        bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
        bool saveConfigItems(FILE *fp);
        virtual bool initProperties();
        const char *getDefaultName();
        bool updateProperties();

      protected:

        bool Connect();
        bool Disconnect();

        void TimerHit();

        virtual IPState Move(DomeDirection dir, DomeMotionCommand operation);
        virtual IPState Park();
        virtual IPState UnPark();                
        virtual bool Abort();

        virtual bool getFullOpenedLimitSwitch();
        virtual bool getFullClosedLimitSwitch();

        // Main Tab
        INumberVectorProperty MoveSteppNP;
        INumber MoveSteppN[2];

        INumberVectorProperty AbsolutePosNP;
        INumber AbsolutePosN[1];

        ISwitchVectorProperty PowerSwitchSP         [MAX_POWER_CHANNELS];
        ISwitch PowerSwitchS                        [MAX_POWER_CHANNELS]    [CHANNEL_STATES];

        // Calibration Tab
        ITextVectorProperty DeviceListTP;
        IText DeviceListT[1];

        ITextVectorProperty DeviceTestTP;
        IText DeviceTestT[1];

        ITextVectorProperty DeviceSelectTP;
        IText DeviceSelectT[3];

        ITextVectorProperty PowerDeviceTP;
        IText PowerDeviceT[MAX_POWER_CHANNELS];

        INumberVectorProperty RoofPropertiesNP;
        INumber RoofPropertiesN[2];

        INumberVectorProperty RoofTravelMSNP;
        INumber RoofTravelMSN[1];

        INumberVectorProperty RoofLimitNP;
        INumber RoofLimitN[2];

        // Power Tab
        ISwitchVectorProperty PowerOnStateSwitchSP  [MAX_POWER_CHANNELS];
        ISwitch PowerOnStateSwitchS                 [MAX_POWER_CHANNELS]    [POWER_STATES];
        
        ISwitchVectorProperty PowerOffStateSwitchSP [MAX_POWER_CHANNELS];
        ISwitch PowerOffStateSwitchS                [MAX_POWER_CHANNELS]    [POWER_STATES];

    private:

        ISState fullOpenLimitSwitch;
        ISState fullClosedLimitSwitch;

        double MotionRequest;
        struct timeval MotionStart;
        double AbsAtStart;
        bool isConnecting;
        bool isAdding;
        void SetAndUpdatePowerDevs();
        bool SetupParams();
        void DefineProperties();
        void DeleteProperties();

        bool Power(ISwitch powerSwitch[], int devNbr, bool forceState, bool stateOn);
        vector<const char*> getDevices();
        void UpdateChannels(int devNbr);
        bool TestDevice(char *devName);
        bool CheckValidDevice(char* text);
        
        bool isParkingAction;
	    bool TestingDevice;
        bool MoveStepp;
        double absTicker;

        float CalcTimeLeft(timeval);
        void setAbsolutePosition();
        bool StopParkingAction(int dir);
        void ParkedStatus(bool status);
};
#endif
