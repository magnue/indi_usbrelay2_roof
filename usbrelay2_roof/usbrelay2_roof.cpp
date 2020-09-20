/*******************************************************************************
 USBRelay2 Roof # INDI driver for controlling usb-relay2 devices
 Copyright(c) 2015 Magnus W. Eriksen

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
#include "usbrelay2_roof.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <cstring>

#include <memory>
#include <algorithm>

#include <libindi/indicom.h>
#include <libindi/indilogger.h>

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

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
        ISInit();
        usbRelay2->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        ISInit();
        usbRelay2->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], 
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
    for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
    {
        PowerSwitchSP[i] = new ISwitchVectorProperty;
        PowerSwitchSP[i]->sp = NULL;

        PowerDeviceTP[i] = new ITextVectorProperty;
    	PowerDeviceTP[i]->tp = NULL;

        ConnectingSwitchSP[i] = new ISwitchVectorProperty;
        ParkingSwitchSP[i] = new ISwitchVectorProperty;
        UnparkSwitchSP[i] = new ISwitchVectorProperty;
    }
    PowerDeviceT[0] = new IText;
    
    setVersion(0,3);
    MotionRequest = 0;
    AbsAtStart = 0;
    isConnecting = true;
    isAdding = false;
    isParkingAction = false;

    SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_PARK);   
}

USBRelay2::~USBRelay2()
{
    for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
    {
        delete PowerDeviceTP[i];
        delete PowerSwitchSP[i];
        delete ConnectingSwitchSP[i];
        delete ParkingSwitchSP[i];
        delete UnparkSwitchSP[i];
    }
}

/************************************************************************************
 * USBRealay2 Roof
* ***********************************************************************************/
bool USBRelay2::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &ActiveDeviceTP);

    IUSaveConfigText(fp, &DeviceSelectTP);
    if (PowerSwitchSP[0]->sp != NULL)
    {
        IUSaveConfigSwitch(fp, &ConnectingEnableSP);
        IUSaveConfigSwitch(fp, &ParkingEnableSP);
        IUSaveConfigSwitch(fp, &UnparkEnableSP);
    }
    for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
    {
        if (PowerSwitchSP[i]->sp != NULL)
        {
            IUSaveConfigText(fp, PowerDeviceTP[i]);
            IUSaveConfigSwitch(fp, ConnectingSwitchSP[i]);
            IUSaveConfigSwitch(fp, ParkingSwitchSP[i]);
            IUSaveConfigSwitch(fp, UnparkSwitchSP[i]);
        }
    }

    IUSaveConfigNumber(fp, &RoofPropertiesNP);
    IUSaveConfigNumber(fp, &RoofTravelMSNP);
    IUSaveConfigNumber(fp, &RoofLimitNP);

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
    
    IUFillText(&DeviceTestT[0],"DEVICE_TEST","test dev (char5x + i)","");
    IUFillTextVector(&DeviceTestTP,DeviceTestT,1,getDeviceName(),"DEVICE_TEST","Device test",
            SETUP_TAB,IP_RW,60,IPS_IDLE);

    IUFillText(&DeviceSelectT[0],"DEVICE_A","dev #A (char5x + i)","");
    IUFillText(&DeviceSelectT[1],"DEVICE_B","dev #B (char5x + i)","");
    IUFillText(&DeviceSelectT[2],"DEVICE_C","dev #C (char5x + i)","");
    IUFillTextVector(&DeviceSelectTP,DeviceSelectT,3,getDeviceName(),"DEVICE_SELECTION","Open/Close devices",
            SETUP_TAB,IP_RW,60,IPS_IDLE);

    IUFillText(PowerDeviceT[0],"POWER_DEVICE_NAME","dev 0 (char5x + i)","");
    IUFillTextVector(PowerDeviceTP[0],PowerDeviceT[0],1,getDeviceName(),"POWER_DEVICE_0","Power device 0",
            SETUP_TAB,IP_RW,60,IPS_IDLE);    

    /************************************************************************************
    * Power setup TAB
    * ***********************************************************************************/
    IUFillSwitch(&ConnectingEnableS[0],"CONNECTING_DISABLE","leave as is",ISS_ON);
    IUFillSwitch(&ConnectingEnableS[1],"CONNECTING_ENABLE","use connect mapping",ISS_OFF);
    IUFillSwitchVector(&ConnectingEnableSP,ConnectingEnableS,0,getDeviceName(),"CONNECTING_SELECT",
            "Connect action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

    IUFillSwitch(&ParkingEnableS[0],"PARKING_DISABLE","leave as is",ISS_ON);
    IUFillSwitch(&ParkingEnableS[1],"PARKING_ENABLE","use park mapping",ISS_OFF);
    IUFillSwitchVector(&ParkingEnableSP,ParkingEnableS,0,getDeviceName(),"PARKING_SELECT",
            "Park action",POWER_TAB,IP_RW,ISR_1OFMANY,0,IPS_OK);

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
        fullOpenLimitSwitch     = ISS_OFF;
        fullClosedLimitSwitch   = ISS_ON;
        SetParked(true);
        AbsolutePosN[0].value   = RoofLimitN[1].value;
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

    isConnecting = true;
    SetTimer(750);

    return true;
}

void USBRelay2::SetAndUpdatePowerDevs()
{
    if (!isConnecting)
        return;
    isConnecting = false;
    DefineProperties();
    if (ConnectingEnableS[1].s == ISS_ON && PowerSwitchSP[0]->sp != NULL)
    {
        IDMessage(getDeviceName(),"Connect power mapping is enabled\n");
        for (int i = 0; PowerSwitchSP[i]->sp != NULL; ++i)
            Power(ConnectingSwitchS[i], i);
    }
    else
        for (int i = 0; PowerSwitchSP[i]->sp != NULL; ++i)
            UpdateChannels(i);
}

const char * USBRelay2::getDefaultName()
{
    return (char *)"USBRelay2 Roof";
}

bool USBRelay2::updateProperties()
{
    INDI::Dome::updateProperties();
    DEBUG(INDI::Logger::DBG_DEBUG,strdup("*****Update properties\n"));

    if (isConnected())
    {
        DefineProperties();
        SetupParms();
    } else
    {
        DeleteProperties();
    }

    return true;
}

void USBRelay2::DefineProperties()
{
    defineSwitch(&ConnectingEnableSP);
    defineSwitch(&ParkingEnableSP);
    defineSwitch(&UnparkEnableSP);
    if (isConnecting)
    {
        defineNumber(&MoveSteppNP);
        defineNumber(&AbsolutePosNP);
        defineNumber(&RoofPropertiesNP);
        defineNumber(&RoofTravelMSNP);
        defineNumber(&RoofLimitNP);
        defineText(&DeviceListTP);
        defineText(&DeviceTestTP);
        defineText(&DeviceSelectTP);
        defineText(PowerDeviceTP[0]);
    }   
    else
    {
        for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
        {
            if (PowerDeviceTP[i]->tp != NULL)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Powerdevice %d, text %s\n", i, PowerDeviceTP[i]->tp->text);
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Powerdevice %d, label %s, name %s\n", i, PowerDeviceTP[i]->label, PowerDeviceTP[i]->name);
            }
            deleteProperty(PowerDeviceTP[i]->name);
            deleteProperty(PowerSwitchSP[i]->name);
            deleteProperty(ConnectingSwitchSP[i]->name);
            deleteProperty(ParkingSwitchSP[i]->name);
            deleteProperty(UnparkSwitchSP[i]->name);
        }
        deleteProperty(ConnectingEnableSP.name);
        deleteProperty(ParkingEnableSP.name);
        deleteProperty(UnparkEnableSP.name);
    }
    bool update = false;
    for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
    {
        if (i == 0)
        {
            defineText(PowerDeviceTP[i]);
        }
        if (i != MAX_POWER_CHANNELS - 1 && PowerDeviceTP[i+1]->tp != NULL)
        {
            update = true;
            defineText(PowerDeviceTP[i+1]);
        }
        else
            DEBUGF(INDI::Logger::DBG_DEBUG,"*****NOT defining powerdevice %d\n", i);
    }
    if (update)
    {
        for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
        {
            if (PowerSwitchSP[i]->sp != NULL)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****DEFINING powerdevice %d\n", i);
                defineSwitch(PowerSwitchSP[i]);
                if (isAdding && !isConnecting)
                    UpdateChannels(i);
            }
        }
        for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
        {
            if (ConnectingSwitchSP[i]->sp != NULL)
            {
                defineSwitch(ConnectingSwitchSP[i]);
            }
        }
        defineSwitch(&ConnectingEnableSP);
        for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
        {
            if (ParkingSwitchSP[i]->sp != NULL)
            {
                defineSwitch(ParkingSwitchSP[i]);
            }
        }
        defineSwitch(&ParkingEnableSP);
        for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
        {
            if (UnparkSwitchSP[i]->sp != NULL)
            {
                defineSwitch(UnparkSwitchSP[i]);
            }
        }
        defineSwitch(&UnparkEnableSP);
    }
    isAdding = false;
}

void USBRelay2::DeleteProperties()
{
    for (int i = 0; i < MAX_POWER_CHANNELS; ++i)
    {
        deleteProperty(PowerDeviceTP[i]->name);
        deleteProperty(PowerSwitchSP[i]->name);
        deleteProperty(ConnectingSwitchSP[i]->name);
        deleteProperty(ParkingSwitchSP[i]->name);
        deleteProperty(UnparkSwitchSP[i]->name);
    }

    deleteProperty(MoveSteppNP.name);
    deleteProperty(AbsolutePosNP.name);
    deleteProperty(RoofPropertiesNP.name);
    deleteProperty(RoofTravelMSNP.name);
    deleteProperty(RoofLimitNP.name);
    deleteProperty(DeviceListTP.name);
    deleteProperty(DeviceTestTP.name);
    deleteProperty(DeviceSelectTP.name);
    deleteProperty(ConnectingEnableSP.name);
    deleteProperty(ParkingEnableSP.name);
    deleteProperty(UnparkEnableSP.name);
}

bool USBRelay2::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    if (strcmp(dev,getDeviceName())==0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****USBRelay::ISNewNumber %s\n", name);

        if (strcmp(name,MoveSteppNP.name)==0)
        {
            // No step motion requested
            if (values[0] == 0 && values[1] == 0)
                return true;

            if (values[0] > 0 && values[1] > 0)
            {
                IDMessage(getDeviceName(), "Can not step in two directions at the same time, %s\n",
                        "set open or close step to zero");
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
                int ret = 0;
                if (isStepOpen)
                {
                    ret = usb.OpenCloseChannel(DeviceSelectT[0].text, true);
                    ret += usb.OpenCloseChannel(DeviceSelectT[1].text, true);
                }
                ret += usb.OpenCloseChannel(DeviceSelectT[2].text, true);
                if (ret != 0)
                    return false;
            }
            double percentOfTravel = (RoofTravelMSN[0].value * 0.01) * (isStepOpen ? values[0] : values[1]);
            IDMessage(getDeviceName(), "Stepping roof %s, for %6.2f milliseconds\n",
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
            
            if (isConnecting)
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

bool USBRelay2::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev,getDeviceName())==0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****USBRelay2::ISNewText %s\n", name);

        string fullString = name;
        string subString = "";
        try {
            subString = fullString.substr(0,12);
        } catch (const std::out_of_range& ex) {
            ; // Nothing to do, expected sometimes when ISNewText is not POWER_DEVICE
        }
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Substr name-substring %s\n", strdup(subString.c_str()));

        if (strcmp(name,DeviceTestTP.name)==0)
        {
            TestingDevice = true;
            // Check for actual device
            if (!CheckValidDevice(texts[0]))
                return false;
            
            unsigned int len = static_cast<unsigned int>(strlen(texts[0]));
            if (len == 0)
                return false;

            IUUpdateText(&DeviceTestTP, texts, names, n);
            
            // Initiate device test, and set timer
            if (!INDI::DefaultDevice::isSimulation())
            {
                int ret = usb.OpenCloseChannel(DeviceTestT[0].text, true);
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
            for (int i = 0; i < n; ++i)
                if (!CheckValidDevice(texts[i]))
                    return false;
 
            IUUpdateText(&DeviceSelectTP, texts, names, n);
            DeviceSelectTP.s = IPS_OK;

            IDSetText(&DeviceSelectTP, NULL);
            return true;
        }
        else if (subString == "POWER_DEVICE")
        {
            // Check for valid device
            if (!CheckValidDevice(texts[0]))
                return false;
  
            string devString = fullString.substr(13,14);
            int devNumber;
            if (std::all_of(devString.begin(), devString.end(), ::isdigit))
                devNumber = atoi(devString.c_str());

            string input = texts[0];
            string devName, channelString;
            int channel;
            unsigned int len = static_cast<unsigned int>(strlen(texts[0]));
            if (len != 0)
            {
                devName = input.substr(0,5);
                channelString = input.substr(6,7);
                if (std::all_of(channelString.begin(), channelString.end(), ::isdigit))
                    channel = atoi(channelString.c_str());
            }

            // POWER_DEVICE remove
            if (len == 0)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Removing device %d\n", devNumber);
                int i;
                bool firstOrLast = (devNumber == MAX_POWER_CHANNELS - 1 
                        || PowerDeviceTP[devNumber + 1]->tp == NULL);
                for (i = 0; !firstOrLast
                        && i != MAX_POWER_CHANNELS - 1 
                        && PowerDeviceTP[i+1]->tp != NULL; ++i)
                {
                    if (i >= devNumber)
                    {
                        if (devNumber != MAX_POWER_CHANNELS - 2
                                && PowerDeviceTP[i+1]->tp != NULL)
                        {
                            *PowerSwitchSP[i] = *PowerSwitchSP[i+1];
                            *ConnectingSwitchSP[i] = *ConnectingSwitchSP[i+1];
                            *ParkingSwitchSP[i] = *ParkingSwitchSP[i+1];
                            *UnparkSwitchSP[i] = *UnparkSwitchSP[i+1];
                            strcpy(PowerSwitchSP[i]->name, ("POWER_SWITCH_" + to_string(i)).c_str());
                            strcpy(ConnectingSwitchSP[i]->name, ("CONNECT_DEVICE_" + to_string(i)).c_str());
                            strcpy(ParkingSwitchSP[i]->name, ("PARKING_DEVICE_" + to_string(i)).c_str());
                            strcpy(UnparkSwitchSP[i]->name, ("UNPARK_DEVICE_" + to_string(i)).c_str());
                        }
                        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Reorder at i == %d\n", i);
                        strcpy(PowerDeviceTP[i]->name, ("POWER_DEVICE_" + to_string(i)).c_str());
                        strcpy(PowerDeviceTP[i]->label, ("Power device " + to_string(i)).c_str());
                        strcpy(PowerDeviceTP[i]->tp->label, ("dev " + to_string(i) + " (char5x + i)").c_str());
                        if (PowerDeviceTP[i+1]->tp->text != NULL)
                            strcpy(PowerDeviceTP[i]->tp->text, PowerDeviceTP[i+1]->tp->text);
                        else
                            PowerDeviceTP[i]->tp->text = NULL;
                        IDSetText(PowerDeviceTP[i], NULL);
                    }
                }
                if (!firstOrLast)
                {
                    PowerDeviceT[i] = NULL;
                    PowerDeviceTP[i]->tp = NULL;
                    if (i == MAX_POWER_CHANNELS - 1 && PowerSwitchSP[i]->sp != NULL)
                    {
                        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Cleanup at i == %d, removing switch %d\n", i, i);
                        PowerSwitchSP[i]->sp = NULL;
                        ConnectingSwitchSP[i]->sp = NULL;
                        ParkingSwitchSP[i]->sp = NULL;
                        UnparkSwitchSP[i]->sp = NULL;

                        PowerDeviceT[i] = new IText;
                        IUFillText(PowerDeviceT[i], "POWER_DEVICE_NAME"
                                , ("dev " + to_string(i) + " (char5x + i)").c_str(), "");
                        IUFillTextVector(PowerDeviceTP[i], PowerDeviceT[i], 1, getDeviceName()
                                , ("POWER_DEVICE_" + to_string(i)).c_str()
                                , ("Power device " + to_string(i)).c_str()
                                , SETUP_TAB, IP_RW, 60, IPS_IDLE);
                    }
                    else
                    {
                        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Cleanup at i == %d, removing switch %d\n", i, i-1);
                        PowerSwitchSP[i-1]->sp = NULL;
                        ConnectingSwitchSP[i-1]->sp = NULL;
                        ParkingSwitchSP[i-1]->sp = NULL;
                        UnparkSwitchSP[i-1]->sp = NULL;
                    }

                    DefineProperties();
                    return true;
                }
                else // "Removing" first or last device
                {
                    if (devNumber == MAX_POWER_CHANNELS - 1) // We are not first, but last
                    {
                        PowerDeviceTP[MAX_POWER_CHANNELS - 1]->tp->text = NULL;
                        PowerSwitchSP[MAX_POWER_CHANNELS - 1]->sp = NULL;
                        ConnectingSwitchSP[MAX_POWER_CHANNELS - 1]->sp = NULL;
                        ParkingSwitchSP[MAX_POWER_CHANNELS - 1]->sp = NULL;
                        UnparkSwitchSP[MAX_POWER_CHANNELS - 1]->sp = NULL;
                        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Remove last switch == %d\n", MAX_POWER_CHANNELS - 1);
                    }
                    else // We are first
                    {
                        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Did not remove powerdevice %d (firstOrLast)\n", devNumber);
                        PowerDeviceTP[0]->tp->text = NULL;
                    }
                    DefineProperties();
                    return true;
                }
            }
            else if (PowerDeviceTP[devNumber]->tp->text != NULL) // Replace device
            {
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Replace deviceNbr == %d,  name  == %s, with deviceName == %s\n"
                        , devNumber, PowerDeviceT[devNumber]->text, texts[0]);
                PowerSwitchS[devNumber][0].s = ISS_OFF;
                PowerSwitchS[devNumber][1].s = ISS_ON;
                strcpy(PowerSwitchSP[devNumber]->name, ("POWER_SWITCH_" + to_string(devNumber)).c_str());
                strcpy(PowerSwitchSP[devNumber]->label, ("Switch " + devName + " channel " + to_string(channel)).c_str());

                ConnectingSwitchS[devNumber][0].s = ISS_OFF;
                ConnectingSwitchS[devNumber][1].s = ISS_ON;
                strcpy(ConnectingSwitchSP[devNumber]->name, ("CONNECT_DEVICE_" + to_string(devNumber)).c_str());
                strcpy(ConnectingSwitchSP[devNumber]->label, ("Connect dev " + devName + " channel " + to_string(channel)).c_str());

                ParkingSwitchS[devNumber][0].s = ISS_OFF;
                ParkingSwitchS[devNumber][1].s = ISS_ON;
                strcpy(ParkingSwitchSP[devNumber]->name, ("PARKING_DEVICE_" + to_string(devNumber)).c_str());
                strcpy(ParkingSwitchSP[devNumber]->label, ("Park dev " + devName + " channel " + to_string(channel)).c_str());

                UnparkSwitchS[devNumber][0].s = ISS_OFF;
                UnparkSwitchS[devNumber][1].s = ISS_ON;
                strcpy(UnparkSwitchSP[devNumber]->name, ("UNPARK_DEVICE_" + to_string(devNumber)).c_str());
                strcpy(UnparkSwitchSP[devNumber]->label, ("Unpark dev " + devName + " channel " + to_string(channel)).c_str());
            }
            else // if (PowerDeviceT[devNumber]->text == NULL)// POWER_DEVICE added
            {
                isAdding = true;
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Adding device %s, channel %d\n", strdup(devName.c_str()), channel);
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****devNumber == %d, texts[0] == %s, PowerDeviceTP[%d]->tp->text == %s"
                        ,devNumber, texts[0], devNumber, PowerDeviceTP[devNumber]->tp->text);
                int vacant;
                for (vacant = 0; vacant < MAX_POWER_CHANNELS; ++vacant) 
                { 
                    DEBUGF(INDI::Logger::DBG_DEBUG,"*****PowerSwitchSP[%d]->sp == %s\n", vacant, PowerSwitchSP[vacant]->sp != NULL ? "(populated)" : "(vacant)"); 
                    DEBUGF(INDI::Logger::DBG_DEBUG,"*****PowerDeviceT[%d] == %s\n", vacant, PowerDeviceT[vacant] != NULL ? "(populated)" : "(vacant)");
                }
                
                for (vacant = 0; vacant != MAX_POWER_CHANNELS
                        && PowerDeviceT[vacant] != NULL; ++vacant) {};
                
                --vacant;
                DEBUGF(INDI::Logger::DBG_DEBUG,"*****Using vacant switch array %d\n", vacant);
                
                if (ConnectingEnableSP.nsp == 0)
                {
                    ConnectingEnableSP.nsp = 2;
                    ParkingEnableSP.nsp = 2;
                    UnparkEnableSP.nsp = 2;
                }
                IUFillSwitch(&PowerSwitchS[vacant][0], "POWER_ON_SWITCH", "connect", ISS_OFF);
                IUFillSwitch(&PowerSwitchS[vacant][1], "POWER_OFF_SWITCH", "disconnect", ISS_ON);
                IUFillSwitchVector(PowerSwitchSP[devNumber], &PowerSwitchS[vacant][0], 2
                        , dev, ("POWER_SWITCH_" + to_string(devNumber)).c_str()
                        , ("Switch " + devName + " channel " + to_string(channel)).c_str()
                        , MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

                IUFillSwitch(&ConnectingSwitchS[vacant][0], "CONNECT_ON_DEV", "connect", ISS_OFF);
                IUFillSwitch(&ConnectingSwitchS[vacant][1], "CONNECT_OFF_DEV", "disconnect", ISS_ON);
                IUFillSwitchVector(ConnectingSwitchSP[devNumber], &ConnectingSwitchS[vacant][0], 2
                        , dev, ("CONNECT_DEVICE_" + to_string(devNumber)).c_str()
                        , ("Connect dev " + devName + " channel " + to_string(channel)).c_str()
                        , POWER_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

                IUFillSwitch(&ParkingSwitchS[vacant][0], "PARK_ON_DEV", "connect", ISS_OFF);
                IUFillSwitch(&ParkingSwitchS[vacant][1], "PARK_OFF_DEV", "disconnect", ISS_ON);
                IUFillSwitchVector(ParkingSwitchSP[devNumber], &ParkingSwitchS[vacant][0], 2
                        , dev, ("PARKING_DEVICE_" + to_string(devNumber)).c_str()
                        , ("Park dev " + devName + " channel " + to_string(channel)).c_str()
                        , POWER_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

                IUFillSwitch(&UnparkSwitchS[vacant][0], "UNPARK_ON_DEV", "connect", ISS_OFF);
                IUFillSwitch(&UnparkSwitchS[vacant][1], "UNPARK_OFF_DEV", "disconnect", ISS_ON);
                IUFillSwitchVector(UnparkSwitchSP[devNumber], &UnparkSwitchS[vacant][0], 2
                        , dev, ("UNPARK_DEVICE_" + to_string(devNumber)).c_str()
                        , ("Unpark dev " + devName + " channel " + to_string(channel)).c_str()
                        , POWER_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
             
                if (vacant != MAX_POWER_CHANNELS - 1)
                {
                    PowerDeviceT[vacant + 1] = new IText;
                    IUFillText(PowerDeviceT[vacant + 1], "POWER_DEVICE_NAME"
                            , ("dev " + to_string(devNumber + 1) + " (char5x + i)").c_str(), "");
                    IUFillTextVector(PowerDeviceTP[devNumber + 1], PowerDeviceT[vacant + 1], 1, getDeviceName()
                            , ("POWER_DEVICE_" + to_string(devNumber + 1)).c_str()
                            , ("Power device " + to_string(devNumber + 1)).c_str()
                            , SETUP_TAB, IP_RW, 60, IPS_IDLE);
                }
            }
            IUUpdateText(PowerDeviceTP[devNumber], texts, names, n);
            PowerDeviceTP[devNumber]->s = IPS_OK;
            IDSetText(PowerDeviceTP[devNumber], NULL);
            DefineProperties();

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
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****USBRelay2::ISNewSwitch %s\n", name);

        ISwitchVectorProperty *updateSP = NULL;
        if (strcmp(name,UnparkEnableSP.name)==0)
        {
            UnparkEnableSP.nsp = 2;
            updateSP = &UnparkEnableSP;
        }
        else if (strcmp(name,ParkingEnableSP.name)==0)
        {
            ParkingEnableSP.nsp = 2;
            updateSP = &ParkingEnableSP;
        }
        else if (strcmp(name,ConnectingEnableSP.name)==0)
        {
            ConnectingEnableSP.nsp = 2;
            updateSP = &ConnectingEnableSP;
        }
        else
        {
            for (int i = 0; i < MAX_POWER_CHANNELS && PowerSwitchSP[i]->sp != NULL; ++i)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "*****Checking switch nbr == %d", i);
                if (strcmp(name,PowerSwitchSP[i]->name)==0)
                {
                    IUUpdateSwitch(PowerSwitchSP[i], states, names, n);
                    if (!Power(PowerSwitchS[i], i))
                        return false;
                    updateSP = PowerSwitchSP[i];
                }
                if (strcmp(name,ConnectingSwitchSP[i]->name)==0)
                    updateSP = ConnectingSwitchSP[i];
                else if (strcmp(name,UnparkSwitchSP[i]->name)==0)
                    updateSP = UnparkSwitchSP[i];
                else if (strcmp(name,ParkingSwitchSP[i]->name)==0)
                    updateSP = ParkingSwitchSP[i];
            }
        }
        if (updateSP != NULL)
        {
            IUUpdateSwitch(updateSP, states, names, n);
            updateSP->s = IPS_OK;
            DEBUGF(INDI::Logger::DBG_DEBUG, "*****SetSwitch->name == %s", updateSP->name);
            IDSetSwitch(updateSP, NULL);
            return true;
        }
        else
            DEBUG(INDI::Logger::DBG_DEBUG, "*****SetSwitch updateSP == NULL!");
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
    // Startup, update after loading config
    else if (isConnecting)
    {
        SetAndUpdatePowerDevs();
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
        int ret = 0;
        if (open)
        {
            ret = usb.OpenCloseChannel(DeviceSelectT[0].text, true);
            ret += usb.OpenCloseChannel(DeviceSelectT[1].text, true);
        }
        ret += usb.OpenCloseChannel(DeviceSelectT[2].text, true);
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
    isParkingAction = true;
    return Move(DOME_CCW, MOTION_START);
}

IPState USBRelay2::UnPark()
{
    IDMessage(getDeviceName(),"USBRelay2 is unparking...\n");
    isParkingAction = true;
    return Move(DOME_CW, MOTION_START);
}

bool USBRelay2::Abort()
{
    // Abort (stop) device test
    if (TestingDevice)
    {
        if (!INDI::DefaultDevice::isSimulation())
        {
	        int ret = usb.OpenCloseChannel(DeviceTestT[0].text, false);
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
        {
            ret = usb.OpenCloseChannel(DeviceSelectT[2].text, false);
            ret += usb.OpenCloseChannel(DeviceSelectT[0].text, false);
            ret += usb.OpenCloseChannel(DeviceSelectT[1].text, false);
        }
        else 
            ret = usb.OpenCloseChannel(DeviceSelectT[2].text, false);
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
        IDMessage(getDeviceName(), "Could not abort, no roof motion...\n");
        return false;
    }

    // Simulated, timerhit is stopped, return
    if (INDI::DefaultDevice::isSimulation())
    {
        IDMessage(getDeviceName(),"Simulated motion stopping...\n");
        return StopParkingAction(abortOpen ? DOME_CW : DOME_CCW);
    }

    // Abort normal motion (Park, Open - Close)
    int ret = usb.OpenCloseChannel(DeviceSelectT[2].text, false);
    if (abortOpen)
    {
        ret += usb.OpenCloseChannel(DeviceSelectT[0].text, false);
        ret += usb.OpenCloseChannel(DeviceSelectT[1].text, false);
    }
    if (ret != 0)
    {
        IDMessage(getDeviceName(),"There was an error trying to stopp %s of roof\n", 
                (abortOpen ? "opening" : "closing") );
        return false;
    }
    return StopParkingAction(abortOpen ? DOME_CW : DOME_CCW);
}

bool USBRelay2::Power(ISwitch powerSwitch[], int devNbr)
{
    ISState onOff;
    onOff = powerSwitch[0].s;
    
    IDMessage(getDeviceName(),"Powering %s device %s\n"
            ,onOff == ISS_ON ? "on" : "off", PowerDeviceT[devNbr]->text);
    if (!INDI::DefaultDevice::isSimulation())
    {
        int ret = usb.OpenCloseChannel(PowerDeviceT[devNbr]->text, onOff == ISS_ON ? true : false);
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****Powering %s device number %d with name %s\n"
                , onOff == ISS_ON ? "on" : "off", devNbr, PowerDeviceT[devNbr]->text);
        if (ret != 0)
            return false;
    }
    else
    {
        PowerSwitchS[devNbr][0].s = onOff;
        PowerSwitchS[devNbr][1].s = onOff == ISS_ON ? ISS_OFF : ISS_ON;
        PowerSwitchSP[devNbr]->s = IPS_OK;
    }
    // If simulatin we assume channelstatus is as intended
    if (INDI::DefaultDevice::isSimulation())
    {
        IDSetSwitch(PowerSwitchSP[devNbr], NULL);
    }
    UpdateChannels(devNbr);
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

    double newAbs = (100 / RoofTravelMSN[0].value) * absMS;
    double diff = absTicker - newAbs;


    // We do not want to update the ApbsolutePos property for any amount of changes.
    // Avoids client being spammed on low speed remot connections.
    if (diff >= 500 || diff <= -500 || timeleft <= 500)
    {
        AbsolutePosN[0].value = newAbs;
        AbsolutePosNP.s = IPS_OK;
        IDSetNumber(&AbsolutePosNP, NULL);
        absTicker = 0;
    } else
        absTicker += newAbs;
}

bool USBRelay2::StopParkingAction(int dir)
{
    bool isClosing = dir == DOME_CCW ? true : false;
    DomeMotionS[dir].s = ISS_OFF;

    if (fullOpenLimitSwitch == ISS_ON || fullClosedLimitSwitch == ISS_ON)
    {
        SetParked(isClosing);
        if (!isParkingAction) // We are not parking, only moving
            return true;
        for (int i = 0; PowerSwitchSP[i]->sp != NULL; ++i)
        {
            if (isClosing && ParkingEnableS[1].s == ISS_ON)
                Power(ParkingSwitchS[i], i);
            else if (!isClosing && UnparkEnableS[1].s == ISS_ON)
                Power(UnparkSwitchS[i], i);
        }
        IDMessage(getDeviceName(),"Roof is %s.\n", isClosing ? "closed" : "opened");
        DomeMotionSP.s = IPS_OK;
        isParkingAction = false;
        return true;
    }
    else
    {
        IDMessage(getDeviceName(),"Aborted %s of roof.\n", isClosing ? "closing" : "opening");
        fullOpenLimitSwitch   = ISS_OFF;
        fullClosedLimitSwitch = ISS_OFF;
        DomeMotionSP.s = IPS_ALERT;
        isParkingAction = false;
        SetParked(!isClosing ? true : false);
        return false;
    }
}

bool USBRelay2::CheckValidDevice(char* text)
{
    DEBUGF(INDI::Logger::DBG_DEBUG,"*****CheckValid texts = %s\n", text);
    unsigned int len = static_cast<unsigned int>(strlen(text));
    if (len == 7 || len == 0)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG,"*****CheckValid len = %d\n", len);
        if (len == 0)
            return true;
            
        string fullString = text;
        string devName = fullString.substr(0,5);
        string channelString = fullString.substr(6,7);
        int channel;
        if (std::all_of(channelString.begin(), channelString.end(), ::isdigit))
            channel = atoi(channelString.c_str());
        else
        {
            IDMessage(getDeviceName(), "Channel == %s, not a number\n", channelString.c_str());
            return false;
        }

        int nbrOfChannels;
        if (channel < 1 || channel > 9)
        {
            IDMessage(getDeviceName(), "You can not spessify channel < 1, or > 8. Channel %d is not valid\n", channel);
            return false;
        }
        else if (!INDI::DefaultDevice::isSimulation() && !usb.TestConnect(strdup(devName.c_str())))
        {
            IDMessage(getDeviceName(),"Failed to connect to device %s, channel %d\n",devName.c_str(), channel);
            IDMessage(getDeviceName(),"Use device test to see if dev name is correct\n");
            return false;
        } 
        else if (!INDI::DefaultDevice::isSimulation() 
                && (nbrOfChannels = usb.GetNumberOfChannelsForDevice(strdup(devName.c_str()))) < channel)
        {
            IDMessage(getDeviceName(), "Failed to connect to device %s, channel %d\n",devName.c_str(), channel);
            IDMessage(getDeviceName(), "Device %s, has only %d channels\n",devName.c_str(), nbrOfChannels);
            return false;
        }
        else
        {
            int i;
            for (i = 0; !TestingDevice 
                    && i != MAX_POWER_CHANNELS - 1 
                    && PowerDeviceTP[i+1]->tp != NULL; ++i)
            {
                if (strcmp(PowerDeviceTP[i]->tp->text, text)==0)
                {
                    IDMessage(getDeviceName(), "Device %s, channel %d is valid, but allready configured as a powerdevice %d\n"
                            , devName.c_str(), channel, i);
                    return false;
                }
            }
            if (i == MAX_POWER_CHANNELS)
            {
                IDMessage(getDeviceName(), "Device %s, channel %d is valid, but there is only room for %d powerdevices\n"
                        ,devName.c_str(), channel, MAX_POWER_CHANNELS);
                return false;
            }
            else if (!isConnecting)
                IDMessage(getDeviceName(), "Device %s, channel %d is a valid device\n",devName.c_str(), channel);
        }
    }
    else
    {
        IDMessage(getDeviceName(),"Device has info with a length of %u, need to be 7. Dev name (5x char) + ([space]Channel)\n",len);
        IDMessage(getDeviceName(),"%s is not a valid device name. Example is (ABCDE 1)\n",text);
        return false;
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

void USBRelay2::UpdateChannels(int devNbr)
{
    if (INDI::DefaultDevice::isSimulation())
        return;

    string input = PowerDeviceT[devNbr]->text;
    string channelString;
    try {
        channelString = input.substr(6,7);
    } catch (const std::out_of_range& ex) {
        return;
    }
    int channel;
    if (std::all_of(channelString.begin(), channelString.end(), ::isdigit))
        channel = atoi(channelString.c_str());
    else
        return;
    vector<bool> channelStatus = usb.GetChannelsForDevice(strdup(input.c_str()));
    vector<bool>::iterator it = channelStatus.begin();
    int i = 1;
    for (; it != channelStatus.end() && i < channel; ++it, ++i) {};

    DEBUGF(INDI::Logger::DBG_DEBUG, "*****Channel == %d, has state = %s. Device is %s", i, *it ? "true" : "false", input.c_str());

    ISState sw = *it ? ISS_ON : ISS_OFF;
    PowerSwitchS[devNbr][0].s = sw;
    PowerSwitchS[devNbr][1].s = sw == ISS_ON ? ISS_OFF : ISS_ON;
    PowerSwitchSP[devNbr]->s = IPS_OK;
    IDSetSwitch(PowerSwitchSP[devNbr], NULL);
    DEBUGF(INDI::Logger::DBG_DEBUG, "*****Updated power switch channel status for powerdevice %s\n", input.c_str());
}
