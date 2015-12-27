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
#include "usbrelay2_roof.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <memory>

#include <indicom.h>

#define SETUP_TAB "Device setup"
#define POWER_TAB "Power setup"

// We declare an auto pointer to USBRelay2.
std::unique_ptr<USBRelay2> usbRelay2(new USBRelay2());

void ISPoll(void *p);

void ISInit()
{
   static int isInit =0;

   if (isInit == 1)
       return;

    isInit = 1;
    if (usbRelay2.get() == 0) usbRelay2.reset(new USBRelay2());

}

void ISGetProperties(const char *dev)
{
        ISInit();
        usbRelay2->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        ISInit();
        usbRelay2->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        usbRelay2->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        usbRelay2->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], 
        char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
    ISInit();
    usbRelay2->ISSnoopDevice(root);
}

USBRelay2::USBRelay2()
{
    fullOpenLimitSwitch   = ISS_ON;
    fullClosedLimitSwitch = ISS_OFF;

    MotionRequest = 0;
    AbsAtStart = 0;
    Startup = true;
    DoFix = false;
    IsParkingAction = false;

    SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_PARK);   
}

USBRelay2::~USBRelay2()
{
}

/************************************************************************************
 * USBRealay2 Roof
* ***********************************************************************************/
bool USBRelay2::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &ActiveDeviceTP);
    IUSaveConfigText(fp, &PortTP);
    IUSaveConfigSwitch(fp, &AutoParkSP);
    IUSaveConfigText(fp, &DeviceSelectTP);
    IUSaveConfigText(fp, &PowerDeviceTP);

    IUSaveConfigNumber(fp, &RoofPropertiesNP);
    IUSaveConfigNumber(fp, &RoofTravelMSNP);
    IUSaveConfigNumber(fp, &RoofLimitNP);

    if (PowerDeviceT[0].text != NULL && strcmp(PowerDeviceT[0].text,"")!=0)
    {
        IUSaveConfigSwitch(fp, &ConnectingEnableSP);
        IUSaveConfigSwitch(fp, &ConnectingSwitchSP);
        IUSaveConfigSwitch(fp, &ConnectingSwitchCh2SP);
        IUSaveConfigSwitch(fp, &ParkingEnableSP);
        IUSaveConfigSwitch(fp, &ParkingSwitchSP);
        IUSaveConfigSwitch(fp, &ParkingSwitchCh2SP);
        IUSaveConfigSwitch(fp, &UnparkEnableSP);
        IUSaveConfigSwitch(fp, &UnparkSwitchSP);
        IUSaveConfigSwitch(fp, &UnparkSwitchCh2SP);
    }

    return true;
}

bool USBRelay2::initProperties()
{
    INDI::Dome::initProperties();

    /************************************************************************************
    * Main Tab
    * ***********************************************************************************/
    IUFillNumber(&MoveSteppN[0], "STEPP_OPEN", "stepp open (%)", "%6.2f", 0, 25, 1, 0);
    IUFillNumber(&MoveSteppN[1], "STEPP_CLOSE", "stepp close (%)", "%6.2f", 0, 25, 1, 0);
    IUFillNumberVector(&MoveSteppNP, MoveSteppN, 2, getDeviceName(), "STEPP_MOVE", "Stepp",
            MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&AbsolutePosN[0], "ABS_POS", "abs position (%)", "%6.2f", -200, 200, 1, 0);
    IUFillNumberVector(&AbsolutePosNP, AbsolutePosN, 1, getDeviceName(), "ABSOLUTE_POSITION",
            "Roof position", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&PowerSwitchS[0],"POWER_SWITCH_1-1","switch 1 ch.1",ISS_OFF);
    IUFillSwitch(&PowerSwitchCh2S[0],"POWER_SWITCH_1-2","switch 1 ch.2",ISS_OFF);
    IUFillSwitch(&PowerSwitchS[1],"POWER_SWITCH_2-1","switch 2 ch.1",ISS_OFF);
    IUFillSwitch(&PowerSwitchCh2S[1],"POWER_SWITCH_2-2","switch 2 ch.2",ISS_OFF);
    IUFillSwitch(&PowerSwitchS[2],"POWER_SWITCH_3-1","switch 3 ch.1",ISS_OFF);
    IUFillSwitch(&PowerSwitchCh2S[2],"POWER_SWITCH_3-2","switch 3 ch.2",ISS_OFF);
    IUFillSwitch(&PowerSwitchS[3],"POWER_SWITCH_4-1","switch 4 ch.1",ISS_OFF);
    IUFillSwitch(&PowerSwitchCh2S[3],"POWER_SWITCH_4-2","switch 4 ch.2",ISS_OFF);

    IUFillSwitchVector(&PowerSwitchSP,PowerSwitchS,0,getDeviceName(),"POWER_SWITCHES-1","Power ch.1",
            MAIN_CONTROL_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
    IUFillSwitchVector(&PowerSwitchCh2SP,PowerSwitchCh2S,0,getDeviceName(),"POWER_SWITCHES-2","Power ch.2",
            "Main Control",IP_RW,ISR_NOFMANY,0,IPS_IDLE);

    /************************************************************************************
    * Calibration Tab
    * ***********************************************************************************/
    IUFillNumber(&RoofPropertiesN[0], "ROOF_SPEED", "motor speed (cm/sec)", "%6.2f", 0, 15, 0.01, 0);
    IUFillNumber(&RoofPropertiesN[1], "ROOF_TRAVEL", "travel in (meters)", "%6.2f", 0, 10, 0.01, 0);
    IUFillNumberVector(&RoofPropertiesNP, RoofPropertiesN, 2, getDeviceName(), "ROOF_PROPERTIES",
            "Roof properties", SETUP_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&RoofTravelMSN[0], "ROOF_TRAVEL_MS", "travel in (ms)", "%6.2f", 0, 10000, 1, 0);
    IUFillNumberVector(&RoofTravelMSNP, RoofTravelMSN, 1, getDeviceName(), "ROOF_TRAVEL_MILLISECONDS",
            "Travel duration", SETUP_TAB, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&RoofLimitN[0], "LIMIT_OPEN", "set open limit (%)", "%6.2f", 50, 150, 1, 100);
    IUFillNumber(&RoofLimitN[1], "LIMIT_CLOSE", "set close limit (%)", "%6.2f", -50, 50, 1, 0);
    IUFillNumberVector(&RoofLimitNP, RoofLimitN, 2, getDeviceName(), "ROOF_TRAVEL_LIMITS",
            "Travel Limits",SETUP_TAB, IP_RW, 60, IPS_OK);

    IUFillText(&DeviceListT[0],"DEVICE_LIST","dev list (char5x)","");
    IUFillTextVector(&DeviceListTP,DeviceListT,1,getDeviceName(),"DEVICES_LIST","Device list",
            SETUP_TAB,IP_RO,60,IPS_IDLE);
    
    IUFillText(&DeviceTestT[0],"DEVICE_TEST","test dev (char5x)","");
    IUFillTextVector(&DeviceTestTP,DeviceTestT,1,getDeviceName(),"DEVICE_TEST","Device test",
            SETUP_TAB,IP_RW,60,IPS_IDLE);

    IUFillText(&DeviceSelectT[0],"DEVICE_OPEN","open dev (char5x)","");
    IUFillText(&DeviceSelectT[1],"DEVICE_CLOSE","close dev (char5x)","");
    IUFillTextVector(&DeviceSelectTP,DeviceSelectT,2,getDeviceName(),"DEVICE_SELECTION","Device select",
            SETUP_TAB,IP_RW,60,IPS_IDLE);

    IUFillText(&PowerDeviceT[0],"POWER_DEVICE_1","dev 1 (char5x)","");
    IUFillText(&PowerDeviceT[1],"POWER_DEVICE_2","dev 2 (char5x)","");
    IUFillText(&PowerDeviceT[2],"POWER_DEVICE_3","dev 3 (char5x)","");
    IUFillText(&PowerDeviceT[3],"POWER_DEVICE_4","dev 4 (char5x)","");
    IUFillTextVector(&PowerDeviceTP,PowerDeviceT,1,getDeviceName(),"POWER_DEVICES","Power devices",
            SETUP_TAB,IP_RW,60,IPS_IDLE);

    /************************************************************************************
    * Power setup TAB
    * ***********************************************************************************/
    IUFillSwitch(&ConnectingSwitchS[0],"CONNECTING_SWITCH_1-1","switch 1 ch.1",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchCh2S[0],"CONNECTING_SWITCH_1-2","switch 1 ch.2",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchS[1],"CONNECTING_SWITCH_2-1","switch 2 ch.1",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchCh2S[1],"CONNECTING_SWITCH_2-2","switch 2 ch.2",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchS[2],"CONNECTING_SWITCH_3-1","switch 3 ch.1",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchCh2S[2],"CONNECTING_SWITCH_3-2","switch 3 ch.2",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchS[3],"CONNECTING_SWITCH_4-1","switch 4 ch.1",ISS_OFF);
    IUFillSwitch(&ConnectingSwitchCh2S[3],"CONNECTING_SWITCH_4-2","switch 4 ch.2",ISS_OFF);

    IUFillSwitchVector(&ConnectingSwitchSP,ConnectingSwitchS,0,getDeviceName(),"CONNECTING_SWITCHES-1",
            "Connect ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
    IUFillSwitchVector(&ConnectingSwitchCh2SP,ConnectingSwitchCh2S,0,getDeviceName(),"CONNECTING_SWITCHES-2",
            "Connect ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);

    IUFillSwitch(&ConnectingEnableS[0],"CONNECTING_DISABLE","leave as is",ISS_ON);
    IUFillSwitch(&ConnectingEnableS[1],"CONNECTING_ENABLE","use connect mapping",ISS_OFF);
    IUFillSwitchVector(&ConnectingEnableSP,ConnectingEnableS,0,getDeviceName(),"CONNECTING_SELECT",
            "Connect action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

    IUFillSwitch(&ParkingSwitchS[0],"PARKING_SWITCH_1-1","switch 1 ch.1",ISS_OFF);
    IUFillSwitch(&ParkingSwitchCh2S[0],"PARKING_SWITCH_1-2","switch 1 ch.2",ISS_OFF);
    IUFillSwitch(&ParkingSwitchS[1],"PARKING_SWITCH_2-1","switch 2 ch.1",ISS_OFF);
    IUFillSwitch(&ParkingSwitchCh2S[1],"PARKING_SWITCH_2-2","switch 2 ch.2",ISS_OFF);
    IUFillSwitch(&ParkingSwitchS[2],"PARKING_SWITCH_3-1","switch 3 ch.1",ISS_OFF);
    IUFillSwitch(&ParkingSwitchCh2S[2],"PARKING_SWITCH_3-2","switch 3 ch.2",ISS_OFF);
    IUFillSwitch(&ParkingSwitchS[3],"PARKING_SWITCH_4-1","switch 4 ch.1",ISS_OFF);
    IUFillSwitch(&ParkingSwitchCh2S[3],"PARKING_SWITCH_4-2","switch 4 ch.2",ISS_OFF);

    IUFillSwitchVector(&ParkingSwitchSP,ParkingSwitchS,0,getDeviceName(),"PARKING_SWITCHES-1",
            "Parking ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
    IUFillSwitchVector(&ParkingSwitchCh2SP,ParkingSwitchCh2S,0,getDeviceName(),"PARKING_SWITCHES-2",
            "Parking ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);

    IUFillSwitch(&ParkingEnableS[0],"PARKING_DISABLE","leave as is",ISS_ON);
    IUFillSwitch(&ParkingEnableS[1],"PARKING_ENABLE","use park mapping",ISS_OFF);
    IUFillSwitchVector(&ParkingEnableSP,ParkingEnableS,0,getDeviceName(),"PARKING_SELECT",
            "Park action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

    IUFillSwitch(&UnparkSwitchS[0],"UNPARK_SWITCH_1-1","switch 1 ch.1",ISS_OFF);
    IUFillSwitch(&UnparkSwitchCh2S[0],"UNPARK_SWITCH_1-2","switch 1 ch.2",ISS_OFF);
    IUFillSwitch(&UnparkSwitchS[1],"UNPARK_SWITCH_2-1","switch 2 ch.1",ISS_OFF);
    IUFillSwitch(&UnparkSwitchCh2S[1],"UNPARK_SWITCH_2-2","switch 2 ch.2",ISS_OFF);
    IUFillSwitch(&UnparkSwitchS[2],"UNPARK_SWITCH_3-1","switch 3 ch.1",ISS_OFF);
    IUFillSwitch(&UnparkSwitchCh2S[2],"UNPARK_SWITCH_3-2","switch 3 ch.2",ISS_OFF);
    IUFillSwitch(&UnparkSwitchS[3],"UNPARK_SWITCH_4-1","switch 4 ch.1",ISS_OFF);
    IUFillSwitch(&UnparkSwitchCh2S[3],"UNPARK_SWITCH_4-2","switch 4 ch.2",ISS_OFF);

    IUFillSwitchVector(&UnparkSwitchSP,UnparkSwitchS,0,getDeviceName(),"UNPARK_SWITCHES-1",
            "Unpark ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
    IUFillSwitchVector(&UnparkSwitchCh2SP,UnparkSwitchCh2S,0,getDeviceName(),"UNPARK_SWITCHES-2",
            "Unpark ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);

    IUFillSwitch(&UnparkEnableS[0],"UNPARK_DISABLE","leave as is",ISS_ON);
    IUFillSwitch(&UnparkEnableS[1],"UNPARK_ENABLE","use unpark mapping",ISS_OFF);
    IUFillSwitchVector(&UnparkEnableSP,UnparkEnableS,0,getDeviceName(),"UNPARK_SELECT",
            "Unpark action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

    SetParkDataType(PARK_NONE);

    addAuxControls();
    return true;
}

bool USBRelay2::SetupParms()
{
    // If we have parking data
    if (InitPark())
    {
        if (isParked())
        {
            fullOpenLimitSwitch   = ISS_OFF;
            fullClosedLimitSwitch = ISS_ON;
            AbsolutePosN[0].value = RoofLimitN[1].value;
        }
        else
        {
            fullOpenLimitSwitch   = ISS_ON;
            fullClosedLimitSwitch = ISS_OFF;
            AbsolutePosN[0].value = RoofLimitN[0].value;
        }
    }
    else // No parking data, default
    {
        fullOpenLimitSwitch = ISS_ON;
        fullClosedLimitSwitch = ISS_OFF;
        AbsolutePosN[0].value = RoofLimitN[0].value;
    }
    AbsolutePosNP.s = IPS_OK;
    IDSetNumber(&AbsolutePosNP, NULL);

    // Get the device list..
    string devices(" - ");
    vector<const char*> devList = getDevices();
    for (vector<const char*>::iterator it = devList.begin(); it != devList.end(); ++it)
    {
        devices.append(*it);
        devices.append(" - ");
    }
    DeviceListT[0].text = strdup(devices.c_str());
    IDSetText(&DeviceListTP, "Connected devices: %s\n", devices.c_str());
    return true;
}

bool USBRelay2::Connect()
{
    if (INDI::DefaultDevice::isSimulation())
        IDMessage(getDeviceName(),"Roof controller initialized (simulated)\n");
    else if (usb.Init() == -1 || usb.GetDevices().empty())
    {
        IDMessage(getDeviceName(),"Roof controller did not initialize...\n");
        if (usb.GetDevices().empty()) 
            IDMessage(getDeviceName(),"No connected devices\n");
        return false;
    } else // Init OK
        IDMessage(getDeviceName(),"Roof controller initialized\n");

    Startup = true;
    SetTimer(1000);

    return true;
}

void USBRelay2::StartupFix()
{
    if (ConnectingEnableS[1].s == ISS_ON && (PowerDeviceT[0].text != NULL && strcmp(PowerDeviceT[0].text,"")!=0))
    {
        IDMessage(getDeviceName(),"Connect power mapping is enabled");
        Power(ConnectingSwitchS, NULL, ConnectingSwitchSP.nsp, 1);
        Power(ConnectingSwitchCh2S, NULL, ConnectingSwitchCh2SP.nsp, 2);
        UpdateChannels(PowerSwitchSP.nsp,NULL);
    }

    // Redifine properties to update after loading config
    DefineProperties(false);
    DefineProperties(true);

}

const char * USBRelay2::getDefaultName()
{
        return (char *)"USBRelay2 Roof";
}

bool USBRelay2::updateProperties()
{
    INDI::Dome::updateProperties();

    if (isConnected())
    {
        DefineProperties(true);
        SetupParms();
    } else
    {
        DefineProperties(false);
    }

    return true;
}

void USBRelay2::DefineProperties(bool define)
{
    if (define)
    {
        defineNumber(&MoveSteppNP);
        defineNumber(&AbsolutePosNP);
        defineSwitch(&PowerSwitchSP);
        defineSwitch(&PowerSwitchCh2SP);
        defineNumber(&RoofPropertiesNP);
        defineNumber(&RoofTravelMSNP);
        defineNumber(&RoofLimitNP);
        defineText(&DeviceListTP);
        defineText(&DeviceTestTP);
        defineText(&DeviceSelectTP);
        defineText(&PowerDeviceTP);
        defineSwitch(&ConnectingSwitchSP);
        defineSwitch(&ConnectingSwitchCh2SP);
        defineSwitch(&ConnectingEnableSP);
        defineSwitch(&ParkingSwitchSP);
        defineSwitch(&ParkingSwitchCh2SP);
        defineSwitch(&ParkingEnableSP);
        defineSwitch(&UnparkSwitchSP);
        defineSwitch(&UnparkSwitchCh2SP);
        defineSwitch(&UnparkEnableSP);
    }
    else
    {
        deleteProperty(MoveSteppNP.name);
        deleteProperty(AbsolutePosNP.name);
        deleteProperty(PowerSwitchSP.name);
        deleteProperty(PowerSwitchCh2SP.name);
        deleteProperty(RoofPropertiesNP.name);
        deleteProperty(RoofTravelMSNP.name);
        deleteProperty(RoofLimitNP.name);
        deleteProperty(DeviceListTP.name);
        deleteProperty(DeviceTestTP.name);
        deleteProperty(DeviceSelectTP.name);
        deleteProperty(PowerDeviceTP.name);
        deleteProperty(ConnectingSwitchSP.name);
        deleteProperty(ConnectingSwitchCh2SP.name);
        deleteProperty(ConnectingEnableSP.name);
        deleteProperty(ParkingSwitchSP.name);
        deleteProperty(ParkingSwitchCh2SP.name);
        deleteProperty(ParkingEnableSP.name);
        deleteProperty(UnparkSwitchSP.name);
        deleteProperty(UnparkSwitchCh2SP.name);
        deleteProperty(UnparkEnableSP.name);
    }
}

bool USBRelay2::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    if (strcmp(dev,getDeviceName())==0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        IDLog("USBRelay::ISNewNumber %s\n", name);

        if (strcmp(name,MoveSteppNP.name)==0)
        {
            // No step motion requested
            if (values[0] == 0 && values[1] == 0)
                return true;

            if (values[0] > 0 && values[1] > 0)
            {
                IDMessage(dev, "Can not step in two directions at the same time, %s",
                        "set open or close step to zero\n");
                MoveSteppNP.s = IPS_ALERT;
                return false;
            }
            // Legal values, let's update
            IUUpdateNumber(&MoveSteppNP, values, names, n);
            // And initiate move
            MoveStepp = true;
            bool isStepOpen;
            if (values[0] > 0) 
                isStepOpen = true;
            else
                isStepOpen = false;

            if (!INDI::DefaultDevice::isSimulation())
            {
                int ret = usb.OpenClose(isStepOpen ? DeviceSelectT[0].text : DeviceSelectT[1].text, true);
                if (ret != 0)
                    return false;
            }
            double percentOfTravel = (RoofTravelMSN[0].value * 0.01) * (isStepOpen ? values[0] : values[1]);
            IDMessage(getDeviceName(), "Stepping roof %s, for %6.2f milliseconds",
                    isStepOpen ? "open" : "close", percentOfTravel);
            
            gettimeofday(&MotionStart, NULL);
            AbsAtStart = AbsolutePosN[0].value * (RoofTravelMSN[0].value * 0.01);
            MotionRequest = percentOfTravel;
            SetTimer(percentOfTravel);

            MoveSteppNP.s = IPS_BUSY;
            
            IDSetNumber(&MoveSteppNP, NULL);
            return true;
        }
        else if (strcmp(name,RoofPropertiesNP.name)==0)
        {
            IUUpdateNumber(&RoofPropertiesNP, values, names, n);
            RoofPropertiesNP.s = IPS_OK;
            
            IDSetNumber(&RoofPropertiesNP, NULL);
            
            // We must also update totalTravelMS
            double travelMS = (RoofPropertiesN[1].value * 1000) / (RoofPropertiesN[0].value * 0.01);
            RoofTravelMSN[0].value = travelMS;
            RoofTravelMSNP.s = IPS_OK;
            IDSetNumber(&RoofTravelMSNP, NULL);

            return true;
        }
        else if (strcmp(name,RoofLimitNP.name)==0)
        {
            IUUpdateNumber(&RoofLimitNP, values, names, n);
            RoofLimitNP.s = IPS_OK;
            
            if (Startup)
            {
                if (AbsolutePosN[0].value == 100)
                    AbsolutePosN[0].value = RoofLimitN[0].value;
                else if (AbsolutePosN[0].value == 0)
                    AbsolutePosN[0].value = RoofLimitN[1].value;
            }
            IDSetNumber(&AbsolutePosNP,NULL);
            IDSetNumber(&RoofLimitNP, NULL);
            return true;
        }

    }
    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Dome::ISNewNumber(dev,name,values,names,n);
}

bool USBRelay2::ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev,getDeviceName())==0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        IDLog("USBRelay2::ISNewText %s\n", name);

        if (strcmp(name,DeviceTestTP.name)==0)
        {
            // Check for actual device
            if (!CheckValidDevice(texts, n))
                return false;
            
            IUUpdateText(&DeviceTestTP, texts, names, n);
            
            // Initiate device test, and set timer
	        TestingDevice = true;
            if (!INDI::DefaultDevice::isSimulation())
            {
                int ret = usb.OpenClose(DeviceTestT[0].text, true);
                if (ret != 0)
                    return false;
            }
            DeviceTestTP.s = IPS_BUSY;
            SetTimer(1500);

            IDSetText(&DeviceTestTP, NULL);
            return true;
        } 
        else if (strcmp(name,DeviceSelectTP.name)==0)
        {
            // Check for valid device
            if (!CheckValidDevice(texts, n))
                return false;
 
            IUUpdateText(&DeviceSelectTP, texts, names, n);
            DeviceSelectTP.s = IPS_OK;

            IDSetText(&DeviceSelectTP, NULL);
            return true;
        }
        else if (strcmp(name,PowerDeviceTP.name)==0)
        {
            // Check for valid device
            if (!CheckValidDevice(texts, n))
                return false;
  
            // Check if there is actually something to update
            bool addPowerDev = false;
            bool deletePowerDev = false;
            for (int i = 0; i < n; ++i)
                // Delete id
                if (strcmp(texts[i], "")==0 && PowerDeviceT[i].text != NULL)
                    deletePowerDev = true;
                // New or updated id
                else if (strcmp(texts[i],"")!=0 && PowerDeviceT[i].text == NULL)
                    addPowerDev = true;
                else if (strcmp(texts[i],"")!=0 && strcmp(texts[i],PowerDeviceT[i].text)!=0)
                    addPowerDev = true;
                
            // Nothing to do
            if (!addPowerDev && !deletePowerDev)
            {
                PowerDeviceTP.s = IPS_BUSY;
                IDSetText(&PowerDeviceTP,NULL);
                return true;
            }
           
            int devNbr = 0;
            // Reorder power devs id and remove empty
            if (deletePowerDev)
            {
                int newList = 0;
                for (int i = 0; i < n; ++i)
                {
                    if (strcmp(texts[i], "")!=0)
                    {
                        texts[newList] = texts[i];
                        texts[i] == "";
                        ++newList;
                    }
                }
                devNbr = newList;
                for (int del = devNbr; del < n; ++del)
                    texts[del] = strdup("");
            }
            // Add new id
            else
            {
                // Find number of power devices
                for (devNbr = 0; devNbr < n && strcmp(texts[devNbr], "")!=0; ++devNbr) {}
            }
            UpdateDynSwitches(devNbr, deletePowerDev, texts, names, n);
            return true;
        }

    }
    return INDI::Dome::ISNewText(dev,name,texts,names,n);
}

bool USBRelay2::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev,getDeviceName())==0)
    {
        // This is for our device
        // Now lets see if it's something we process hers
        IDLog("USBRelay2::ISNewSwitch %s\n", name);

        ISwitchVectorProperty *updateSP = NULL;
        if (strcmp(name,PowerSwitchSP.name)==0)
        {
            if (!Power(PowerSwitchS, states, n, 1))
                return false;
            updateSP = &PowerSwitchSP;
        }
        else if (strcmp(name,PowerSwitchCh2SP.name)==0)
        {
            if (!Power(PowerSwitchCh2S, states, n, 2))
                return false;
            updateSP = &PowerSwitchCh2SP;
        }
        else if (strcmp(name,ConnectingSwitchSP.name)==0)
            updateSP = &ConnectingSwitchSP;
        else if (strcmp(name,ConnectingSwitchCh2SP.name)==0)
            updateSP = &ConnectingSwitchCh2SP;
        else if (strcmp(name,UnparkSwitchSP.name)==0)
            updateSP = &UnparkSwitchSP;
        else if (strcmp(name,UnparkSwitchCh2SP.name)==0)
            updateSP = &UnparkSwitchCh2SP;
        else if (strcmp(name,ParkingSwitchSP.name)==0)
            updateSP = &ParkingSwitchSP;
        else if (strcmp(name,ParkingSwitchCh2SP.name)==0)
            updateSP = &ParkingSwitchCh2SP;
        else if (strcmp(name,UnparkEnableSP.name)==0)
            updateSP = &UnparkEnableSP;
        else if (strcmp(name,ParkingEnableSP.name)==0)
            updateSP = &ParkingEnableSP;
        else if (strcmp(name,ConnectingEnableSP.name)==0)
            updateSP = &ConnectingEnableSP;
        
        if (updateSP != NULL)
        {
            IUUpdateSwitch(updateSP, states, names, n);
            updateSP->s = IPS_OK;
            IDSetSwitch(updateSP,NULL);
            return true;
        }
    }
    return INDI::Dome::ISNewSwitch(dev,name,states,names,n);
}

bool USBRelay2::Disconnect()
{
    return true;
}


void USBRelay2::TimerHit()
{
    //  No need to reset timer if we are not connected anymore 
    if (isConnected() == false) return;    

    // USBRelay2 Roof is opening
    if (DomeMotionS[DOME_CW].s == ISS_ON)
    {
        if (getFullOpenedLimitSwitch())
            Abort();
        else
        {
            setAbsulutePosition();
            SetTimer(100);
        }
    }
    // USBRelay2 Roof is closing
    else if (DomeMotionS[DOME_CCW].s == ISS_ON)
    {
        if (getFullClosedLimitSwitch())
            Abort();
        else
        {
            setAbsulutePosition();
            SetTimer(100);
        }
    }
    // We at mowing a step (%)
    else if (MoveStepp)
    {
        if (Abort())
        {
            setAbsulutePosition();
            MoveStepp = false;
        }
    }
    // We are testing device
    else if (TestingDevice)
    {
	    if (Abort())
	        TestingDevice = false;
    }
    // Startup hack
    else if (Startup)
    {
        StartupFix();
        Startup = false;
    }

}

IPState USBRelay2::Move(DomeDirection dir, DomeMotionCommand operation)
{
    if (operation == MOTION_START)
    {
        // DOME_CW --> OPEN. If we are ask to "open" while we are fully opened, 
        // as the limit switch indicates, then we simply return false.
        if (dir == DOME_CW && fullOpenLimitSwitch == ISS_ON)
        {
            IDMessage(getDeviceName(),"Roof is already fully opened.\n");
            return IPS_ALERT;
        }
        else if (dir == DOME_CW && getWeatherState() == IPS_ALERT)
        {
            IDMessage(getDeviceName(),"Weather conditions are in the danger zone. Cannot open roof.\n");
            return IPS_ALERT;
        }
        else if (dir == DOME_CCW && fullClosedLimitSwitch == ISS_ON)
        {
            IDMessage(getDeviceName(),"Roof is already fully closed.\n");
            return IPS_ALERT;
        }
        // We are changing direction! 
        // Must abort current motion before setting new direction to avoid short circut
        else if ( (DomeMotionS[DOME_CW].s == ISS_ON && dir == DOME_CCW)
                || (DomeMotionS[DOME_CCW].s == ISS_ON && dir == DOME_CW) )
            Abort();

        double travelScale;
        if (dir == DOME_CW)
            travelScale = RoofLimitN[0].value / 100;
        else
            travelScale = (100 - RoofLimitN[1].value) / 100;
        AbsAtStart = AbsolutePosN[0].value * (RoofTravelMSN[0].value * 0.01);

        double modify = 0;
        if (fullOpenLimitSwitch == ISS_OFF && fullClosedLimitSwitch == ISS_OFF);
        {
            if (dir == DOME_CW)
                modify = AbsAtStart;
            else
                modify = RoofTravelMSN[0].value - AbsAtStart;
        }

        MotionRequest = (RoofTravelMSN[0].value * travelScale) - modify;

        fullOpenLimitSwitch   = ISS_OFF;
        fullClosedLimitSwitch = ISS_OFF;

        DomeMotionS[dir].s = ISS_ON;
        DomeMotionSP.s = IPS_BUSY;
        gettimeofday(&MotionStart,NULL);

        // Simulation enabeled, set the timer and return
        if (INDI::DefaultDevice::isSimulation())
        {
            IDMessage(getDeviceName(),"Simulating motion for %6.2f millinseconds\n", MotionRequest);
            SetTimer(100);
            return IPS_BUSY;
        }

        // usb.OpenClose will return != 0 if USBInterface cannot open all channels for device
        bool open = (dir == DOME_CW) ? true : false;
        int ret = usb.OpenClose(open ? DeviceSelectT[0].text : DeviceSelectT[1].text, true);
        if (ret != 0)
        {
            IDMessage(getDeviceName(),"Is open and close devices connected?\n");
            IDMessage(getDeviceName(),"There was an error trying to %s roof\n", (open ? "open" : "close") );
            return IPS_ALERT;
        }
        IDMessage(getDeviceName(),"Moving roof for %6.2f milliseconds\n", MotionRequest);
        SetTimer(100);       
        return IPS_BUSY;
    }
    else
        Abort();

    return IPS_ALERT;
}

IPState USBRelay2::Park()
{    
    IDMessage(getDeviceName(),"USBRelay2 is parking...\n");
    IsParkingAction = true;
    return Move(DOME_CCW, MOTION_START);
}

IPState USBRelay2::UnPark()
{
    IDMessage(getDeviceName(),"USBRelay2 is unparking...\n");
    IsParkingAction = true;
    return Move(DOME_CW, MOTION_START);
}

bool USBRelay2::Abort()
{
    // Abort (stop) device test
    if (TestingDevice)
    {
        if (!INDI::DefaultDevice::isSimulation())
        {
	        int ret = usb.OpenClose(DeviceTestT[0].text, false);
	        if (ret != 0)
	            return false;
        }
        DeviceTestTP.s = IPS_OK;
        IDSetText(&DeviceTestTP, "Device test done on device: %s\n", DeviceTestT[0].text);

	    return true;
    } 
    // Abort motion Move Step (%), open or close, and clean up
    else if (MoveStepp)
    {
        int ret;
        if (MoveSteppN[0].value > 0)
            ret = usb.OpenClose(DeviceSelectT[0].text, false);
        else 
            ret = usb.OpenClose(DeviceSelectT[1].text, false);
        if (ret != 0 && !INDI::DefaultDevice::isSimulation())
            return false;
        else {
            MoveSteppNP.s = IPS_OK;
            IDSetNumber(&MoveSteppNP, NULL);
            return true;
        }
    }

    bool abortOpen;
    if (DomeMotionS[DOME_CW].s == ISS_ON && DomeMotionS[DOME_CCW].s == ISS_OFF)
        abortOpen = true;
    else if (DomeMotionS[DOME_CCW].s == ISS_ON && DomeMotionS[DOME_CW].s == ISS_OFF)
        abortOpen = false;
    else
    {
        IDMessage(getDeviceName(), "Could not abort, no roof motion...");
        return false;
    }

    // Simulated, timerhit is stopped, return
    if (INDI::DefaultDevice::isSimulation())
    {
        IDMessage(getDeviceName(),"Simulated motion stopping...\n");
        return StopParkingAction(abortOpen ? DOME_CW : DOME_CCW);
    }

    // Abort normal motion (Park, Open - Close)
    int ret = usb.OpenClose(abortOpen ? DeviceSelectT[0].text : DeviceSelectT[1].text, false);
    if (ret != 0)
    {
        IDMessage(getDeviceName(),"There was an error trying to stopp %s of roof\n", 
                (abortOpen ? "opening" : "closing") );
        return false;
    }
        
    return StopParkingAction(abortOpen ? DOME_CW : DOME_CCW);
}

bool USBRelay2::Power(ISwitch powerSwitch[], ISState *newStates, int n, int channel)
{
    IDMessage(getDeviceName(),"Powering on / off devices on channel %d",channel);

    for (int i = 0; i < n; ++i)
    {
        if (newStates == NULL || powerSwitch[i].s != newStates[i])
        {
            ISState onOff;
            // We are not calling Power from ISNewSwitch if newStates == NULL
            if (newStates == NULL)
                onOff = powerSwitch[i].s;
            else
                onOff = newStates[i];
            
            if (!INDI::DefaultDevice::isSimulation())
            {
                int ret = usb.OpenCloseChannel(PowerDeviceT[i].text, onOff == ISS_ON ? true : false, channel);
                if (ret != 0)
                    return false;
            }
            else
            {
                if (channel == 1)
                    PowerSwitchS[i].s = onOff;
                else
                    PowerSwitchCh2S[i].s = onOff;
            }
        }
    }
    // If simulatin we assume cannelstatus is as intended
    if (INDI::DefaultDevice::isSimulation())
    {
        IDSetSwitch(&PowerSwitchSP, NULL);
        IDSetSwitch(&PowerSwitchCh2SP, NULL);
    }
    return true;
}

float USBRelay2::CalcTimeLeft(timeval start)
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now,NULL);

    timesince=(double)(now.tv_sec * 1000.0 + now.tv_usec/1000) 
        - (double)(start.tv_sec * 1000.0 + start.tv_usec/1000);
    timeleft=MotionRequest-timesince;
    return timeleft;
}

bool USBRelay2::getFullOpenedLimitSwitch()
{
    double timeleft = CalcTimeLeft(MotionStart);

    if (timeleft <= 0)
    {
        fullOpenLimitSwitch = ISS_ON;
        return true;
    }
    else
        return false;
}

bool USBRelay2::getFullClosedLimitSwitch()
{
    double timeleft = CalcTimeLeft(MotionStart);

    if (timeleft <= 0)
    {
        fullClosedLimitSwitch = ISS_ON;
        return true;
    }
    else
        return false;
}

void USBRelay2::setAbsulutePosition()
{
    int dir = DOME_CW;
    if (MoveStepp)
    {
        if (MoveSteppN[0].value == 0)
            dir = DOME_CCW;
    }
    else if (DomeMotionS[DOME_CCW].s == ISS_ON)
        dir = DOME_CCW;
    
    double timeleft = CalcTimeLeft(MotionStart);
    double absMS;
    if (dir == DOME_CW)
        absMS = AbsAtStart + (MotionRequest - timeleft); 
    else
        absMS = AbsAtStart - (MotionRequest - timeleft);
    AbsolutePosN[0].value = (100 / RoofTravelMSN[0].value) * absMS;
    AbsolutePosNP.s = IPS_OK;
    IDSetNumber(&AbsolutePosNP, NULL);
}

bool USBRelay2::StopParkingAction(int dir)
{
    bool isClosing = dir == DOME_CCW ? true : false;
    DomeMotionS[dir].s = ISS_OFF;

    if (fullOpenLimitSwitch == ISS_ON || fullClosedLimitSwitch == ISS_ON)
    {
        SetParked(isClosing);
        if (!IsParkingAction) // We are not parking, only moving
            return true;
        else if (isClosing && ParkingEnableS[1].s == ISS_ON)
        {
            Power(ParkingSwitchS, NULL, ParkingSwitchSP.nsp, 1);
            Power(ParkingSwitchCh2S, NULL, ParkingSwitchCh2SP.nsp, 2);
            UpdateChannels(PowerSwitchSP.nsp,NULL);
        } 
        else if (!isClosing && UnparkEnableS[1].s == ISS_ON)
        {
            Power(UnparkSwitchS, NULL, UnparkSwitchSP.nsp, 1);
            Power(UnparkSwitchCh2S, NULL, UnparkSwitchCh2SP.nsp, 2);
            UpdateChannels(PowerSwitchSP.nsp,NULL);
        }
        IDMessage(getDeviceName(),"Roof is %s.\n", isClosing ? "closed" : "opened");
        DomeMotionSP.s = IPS_OK;
        IsParkingAction = false;
        return true;
    }
    else
    {
        IDMessage(getDeviceName(),"Aborted %s of roof.\n", isClosing ? "closing" : "opening");
        fullOpenLimitSwitch   = ISS_OFF;
        fullClosedLimitSwitch = ISS_OFF;
        DomeMotionSP.s = IPS_ALERT;
        IsParkingAction = false;
        SetParked(!isClosing ? true : false);
        return false;
    }
}

bool USBRelay2::CheckValidDevice(char *texts[], int n)
{
    for (int i = 0; i < n; ++i)
    {
        unsigned int len = static_cast<unsigned int>(strlen(texts[i]));
        if (len == 5 || len == 0)
        {
            if (len == 0)
                return true;
            bool ret = usb.TestConnect(texts[i]);
            if (!ret && !INDI::DefaultDevice::isSimulation())
            {
                IDMessage(getDeviceName(),"Use device test to see if dev name is correct");
                IDMessage(getDeviceName(),"Failed to connect to device %s",texts[i]);
                return false;
            }
            else
                IDMessage(getDeviceName(),"%s status OK",texts[i]);
        }
        else
        {
            IDMessage(getDeviceName(),"Device has name with a length of %u, need to be 5",len);
            IDMessage(getDeviceName(),"%s is not a valid device name.",texts[i]);
            return false;
        }
    }
    return true;
}

vector<const char*> USBRelay2::getDevices()
{
    // Is simulated, populate list with fake devices
    if (INDI::DefaultDevice::isSimulation())
    {
        vector<const char*> simDevices;
        simDevices.push_back(strdup("ABCDE"));
        simDevices.push_back(strdup("FGHIJ"));
        simDevices.push_back(strdup("KLMNO"));
        simDevices.push_back(strdup("RGTFV"));
        return simDevices;
    }
    // Get devices from usb_interface and return
    return usb.GetDevices();
}

void USBRelay2::UpdateChannels(int devNbr, char *texts[])
{
    if (INDI::DefaultDevice::isSimulation())
        return;
    for (int j = 0; j < devNbr; ++j)
    {
        char *devName = texts == NULL ? PowerDeviceT[j].text : texts[j];
        vector<unsigned int> channelStatus = usb.GetChannelsForDevice(devName,2);
        vector<unsigned int>::iterator it = channelStatus.begin();
        for (int i = 0; it != channelStatus.end(); ++it, ++i)
        {
            ISState sw = *it != 0 ? ISS_ON : ISS_OFF;
            if (i % 2 == 0)
                PowerSwitchS[j].s = sw;
            else
                PowerSwitchCh2S[j].s = sw;
        }
    }
    IDSetSwitch(&PowerSwitchSP, NULL);
    IDSetSwitch(&PowerSwitchCh2SP, NULL);
    IDMessage(getDeviceName(), "Updated power switch channel status");
}

void USBRelay2::UpdateDynSwitches(int devNbr, bool deletePowerDev, char *texts[], char *names[], int n)
{
    // Fill text vector and update
    if (devNbr != 5)
    {
        if (deletePowerDev) // We are deleting power device. Update vector before 'Fill' POWER_DEVICES
            IUUpdateText(&PowerDeviceTP, texts, names, n);
        
        vector<ISwitchVectorProperty> svToUpdate;
        devNbr = devNbr < MAX_POWER_DEVS ? devNbr : MAX_POWER_DEVS;

        // Update main tab switches
        IUFillSwitchVector(&PowerSwitchSP, PowerSwitchS, devNbr, getDeviceName(),"POWER_SWITCHES-1",
                "Power ch.1",MAIN_CONTROL_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&PowerSwitchCh2SP,PowerSwitchCh2S,devNbr,getDeviceName(),"POWER_SWITCHES-2",
                "Power ch.2","Main Control",IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        UpdateChannels(devNbr,texts);
        svToUpdate.push_back(PowerSwitchSP);
        svToUpdate.push_back(PowerSwitchCh2SP);

        // Update Power tab switches
        // Connecting
        IUFillSwitchVector(&ConnectingSwitchSP,ConnectingSwitchS,devNbr,getDeviceName(),
                "CONNECTING_SWITCHES-1","Connecting ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&ConnectingSwitchCh2SP,ConnectingSwitchCh2S,devNbr,getDeviceName(),
                "CONNECTING_SWITCHES-2","Connecting ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&ConnectingEnableSP,ConnectingEnableS,(devNbr != 0 ? 2: 0),getDeviceName(),
                "CONNECTING_SELECT","Connect action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);
        svToUpdate.push_back(ConnectingSwitchSP);
        svToUpdate.push_back(ConnectingSwitchCh2SP);
        svToUpdate.push_back(ConnectingEnableSP);

        // Parking
        IUFillSwitchVector(&ParkingSwitchSP,ParkingSwitchS,devNbr,getDeviceName(),
                "PARKING_SWITCHES-1","Parking ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&ParkingSwitchCh2SP,ParkingSwitchCh2S,devNbr,getDeviceName(),
                "PARKING_SWITCHES-2","Parking ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&ParkingEnableSP,ParkingEnableS,(devNbr != 0 ? 2: 0),getDeviceName(),
                "PARKING_SELECT","Park action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);
        svToUpdate.push_back(ParkingSwitchSP);
        svToUpdate.push_back(ParkingSwitchCh2SP);
        svToUpdate.push_back(ParkingEnableSP);

        // Unpark
        IUFillSwitchVector(&UnparkSwitchSP,UnparkSwitchS,devNbr,getDeviceName(),
                "UNPARK_SWITCHES-1","Unpark ch.1",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&UnparkSwitchCh2SP,UnparkSwitchCh2S,devNbr,getDeviceName(),
                "UNPARK_SWITCHES-2","Unpark ch.2",POWER_TAB,IP_RW,ISR_NOFMANY,0,IPS_IDLE);
        IUFillSwitchVector(&UnparkEnableSP,UnparkEnableS,(devNbr != 0 ? 2 : 0),getDeviceName(),
                "UNPARK_SELECT","Unpark action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);
        svToUpdate.push_back(UnparkSwitchSP);
        svToUpdate.push_back(UnparkSwitchCh2SP);
        svToUpdate.push_back(UnparkEnableSP);
        
        vector<ISwitchVectorProperty>::iterator it = svToUpdate.begin();
        for (; it != svToUpdate.end(); ++it)
        {
            deleteProperty(&*it->name);
            defineSwitch(&*it);
        }

        // Update Calib tab - POWER_DEVICES
        devNbr = devNbr != MAX_POWER_DEVS ? devNbr + 1 : MAX_POWER_DEVS;
        IUFillTextVector(&PowerDeviceTP, PowerDeviceT, devNbr, getDeviceName(),
            "POWER_DEVICES","Power devices",SETUP_TAB,IP_RW,60,IPS_OK);
        
        if (!deletePowerDev) // We are adding power device. Update vector after 'Fill' POWER_DEVICES
            IUUpdateText(&PowerDeviceTP, texts, names, n);

        IDSetText(&PowerDeviceTP,NULL);
        deleteProperty(PowerDeviceTP.name);
        defineText(&PowerDeviceTP);
    }
}
