/*******************************************************************************
 USBRelay2 Roof # INDI driver for controlling usb-relay2 devices
 Copyright(c) 2015 Magnus W. Eriksen

 based on: RollOff Roof by:
 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

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

#ifndef USBRelay2_H
#define USBRelay2_H

#include <libindi/indidome.h>
#include "usb_interface.h"

/*  Some headers we need */
#include <math.h>
#include <sys/time.h>

#define MAX_POWER_DEVS 4

class USBRelay2 : public INDI::Dome
{

    public:
        USBRelay2();
        virtual ~USBRelay2();

        USBInterface usb;

        bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
        bool ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int n);
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

        ISwitchVectorProperty PowerSwitchSP;
        ISwitch PowerSwitchS[MAX_POWER_DEVS];

        ISwitchVectorProperty PowerSwitchCh2SP;
        ISwitch PowerSwitchCh2S[MAX_POWER_DEVS];

        // Calibration Tab
        ITextVectorProperty DeviceListTP;
        IText DeviceListT[1];

        ITextVectorProperty DeviceTestTP;
        IText DeviceTestT[1];

        ITextVectorProperty DeviceSelectTP;
        IText DeviceSelectT[2];

        ITextVectorProperty PowerDeviceTP;
        IText PowerDeviceT[MAX_POWER_DEVS];

        INumberVectorProperty RoofPropertiesNP;
        INumber RoofPropertiesN[2];

        INumberVectorProperty RoofTravelMSNP;
        INumber RoofTravelMSN[1];

        INumberVectorProperty RoofLimitNP;
        INumber RoofLimitN[2];

        // Power Tab
        ISwitchVectorProperty ConnectingSwitchSP;
        ISwitch ConnectingSwitchS[MAX_POWER_DEVS];

        ISwitchVectorProperty ConnectingSwitchCh2SP;
        ISwitch ConnectingSwitchCh2S[MAX_POWER_DEVS];

        ISwitchVectorProperty ConnectingEnableSP;
        ISwitch ConnectingEnableS[2];

        ISwitchVectorProperty ParkingSwitchSP;
        ISwitch ParkingSwitchS[MAX_POWER_DEVS];

        ISwitchVectorProperty ParkingSwitchCh2SP;
        ISwitch ParkingSwitchCh2S[MAX_POWER_DEVS];

        ISwitchVectorProperty ParkingEnableSP;
        ISwitch ParkingEnableS[2];

        ISwitchVectorProperty UnparkSwitchSP;
        ISwitch UnparkSwitchS[MAX_POWER_DEVS];

        ISwitchVectorProperty UnparkSwitchCh2SP;
        ISwitch UnparkSwitchCh2S[MAX_POWER_DEVS];

        ISwitchVectorProperty UnparkEnableSP;
        ISwitch UnparkEnableS[2];

    private:

        ISState fullOpenLimitSwitch;
        ISState fullClosedLimitSwitch;

        double MotionRequest;
        struct timeval MotionStart;
        double AbsAtStart;
        bool Startup;
        bool DoFix;
        void StartupFix();
        bool SetupParms();
        void DefineProperties(bool define);

        bool Power(ISwitch powerSwitch[], ISState *newStates, int n, int channel);
        void UpdateDynSwitches(int devNbr, bool deletePowerDev, char *texts[], char *names[], int n);
        vector<const char*> getDevices();
        void UpdateChannels(int devNbr, char *texts[]);
        bool TestDevice(char *devName);
        bool CheckValidDevice(char *texts[], int n);
        
        bool IsParkingAction;
	    bool TestingDevice;
        bool MoveStepp;

        float CalcTimeLeft(timeval);
        void setAbsulutePosition();
        bool  StopParkingAction(int dir);
        void ParkedStatus(bool status);
};

#endif
