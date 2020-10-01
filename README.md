### INDI USBRelay2 Roof

#### Summary
INDI USBRelay2 Roof is a Dome driver compatible with INDI Control Panel and INDI clients. (Tested only with EKOS in KStars)
The project has been compiled and tested on Linux (Ubuntu) x86_64, Linux (Lubuntu) armv7l (ODROID C1+), Linux (Raspbian) arm (Raspberry PI 1 model B).
The "driver" uses the usb-relay-hid API to enable remote controlling of usbrelay devices, as roof controller and power switches. 

To open and close roof three relay channels are used, #A, #B and #C. Channels #A and #B uses the normally open (NO) and normally closed (NC) layout of the relay channels, to flip the positive and negative phase. Channel #C acts as a single phase switch to the DC motor, to start and stop roof motion. It is possible to calibrate the roof travel with motor speed, roof travel and travel limits, but it is advisable to also have physical cut off switches in the fully opened and fully closed position to avoid binding. On the sketch below, #1 is a push-off fully opened limit switch, that limits roof opening further than a set limit. #2 is a push-off fully closed limit switch.

You can also use a garage door opener or similar controller. Just override the open = #3 button, and close = #4 button.

DC motor style connection
![connect](doc-media/usbrelay2-connect.jpg)

Garage door style connection
![connect](doc-media/usbrelay2-connect-garage.jpg)

In INDI USBRelay2 Roof >= v0.3 both 1, 2, 4 and 8 channel devices is supported, and they are all resonably prized on [Ebay](http://www.ebay.com/sch/i.html?_odkw=5v+usb+relay+programmable+compute+controll&_osacat=0&_from=R40&_trksid=p2045573.m570.l1313.TR0.TRC0.H0.X5v+usb+relay+programmable+computer+controll.TRS0&_nkw=5v+usb+relay+programmable+computer+controll&_sacat=0), or
[AliExpress](http://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20160504000929&SearchText=5v+usb+relay+programmable+computer+controll).
Note that it is the RED version that is supported and not the blue. For more info see [usb-relay-hid](https://github.com/pavel-a/usb-relay-hid), and the included links

When power devices is added, you can power channels on and off in the Main Control tab. You also get a power tab that allows you to map the on and off states for all channels, when connecting, when parking, and when unparking the roof. The power switches in the Main tab will always be updated and reflect the actual state of the channel.

It's also worth noting that the 1 and 2 channel boards has a "optional" -5v +5v input. This allows the device to remember channel states when the server reeboots or is powered off. The 4 and 8 channel devices must have a 12v powersupply connected to the -12v +12v input.

INID USBRelay2 Roof does support simulation. You can compile it and see that it works on your system before considering getting the devices. Just enable simulation in the Options tab before connecting.


#### Known issues
* If Open or Close is aborted using the 'Abort' button, the switch status will not reset. This means that if aborting 'Open/Close' the 'Open/Close' button must be klicked twice to resume 'Open/Close'. This does not happend when Parking or Unparking, nor if 'Open/Close' is aborted with a second klick on 'Open/Close'. Aborting Open and resuming to Close works as expexted, and vice versa

#### Attributions
* This project could not be done without the existing libindi project or the usb-relay-hid API.
* This project is 3rd party and dependent on the mentioned projects, and licensed accordingly. See LICENSE, COPYING.GPL and COPYING.BSD.
* Some files have additional lisence information, see file headers.

#### Installing dependencies
##### libindi and it's dependencies
* To build libindi from source [see instructions on indilib.org](http://www.indilib.org/forum/general/210-howto-building-latest-libindi-ekos.html)

##### libusb_relay_device.so [usb_relay_hid], and it's dependencies
```
mkdir ~/Projects
cd ~/Projects
git clone https://github.com/pavel-a/usb-relay-hid.git
cd usb-relay-hid/commandline/makemake/
sudo apt-get install libusb-dev
make
sudo cp usb_relay_device.so /usr/local/lib/libusb_relay_device.so
cd ../../lib/
sudo cp usb_relay_device.h /usr/local/include/usb_relay_device.h
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usr_local_lib.conf
sudo ldconfig
```

#### Install INDI USBRelay2 Roof
```
cd ~/Projects
git clone https://github.com/magnue/indi_usbrelay2_roof.git
cd indi_usbrelay2_roof
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr . ..
sudo make install
```
#### Update
```
cd ~/Projects/indi_usbrelay2_roof/
git pull
cd build
sudo make install
```

#### Example of use

##### connect and calibrate roof
* Connect.. The driver will assume the roof is closed (parked) when connecting the first time.
* In this image I have setup some of the power switches, but other than that, this is how it will look on first connect.

![connect](doc-media/1-maintab-parked.jpg)

* Enter the #A, #B, and #C channel. 
* Use the device test to see whitch physical device belongs to the device strings(s). The device test will turn one channel on for 1.5 sec and off again. Take a note of the result for future reference.
* Set the motor speed (cm/sec) and roof travel (meters). This will calculate the total travel in milliseconds.
* For first run set the open limit to 75%, and close limit to 25%. this will make the roof not fully open and close.
* Note that the format (char5x + i) is the five character long device string, then space [_], and the channel number. [ABCDE 1]
* Unpark the roof so it opens to 75%
* Use the stepp open (%) to fully open the roof, and note the abs position (as this will be your open limit)


* Park the roof so it will close to 25%
* Use stepp close to find the abs position for fully closed position.
* Set your calibrated travel limits in Device Setup.
* As a note the difference between the open, close (Motion) and the park, unpark (Parking) switches is that the (Motion) switches will not initiate the Power mapping.

![power-dev-connect](doc-media/4-devicetab-10channels.jpg)

##### connect and set power devices
* You can also add one or more (max 10) power channels. In this example i added seven.
* For robustness and ease of 'service' there are now 10 fixed power devices, and you will have to keep track of them your self. This makes the code for this driver a lot nicer.


* The Power Setup tab can be used to set the desired states for connection, parking and unparking. Default is leave everything as is, and enable mapping.
* With this new setup you can fine tune with channel is powered on, powered off or left alone when connecting, or unparking and parking the dome roof.
* If you leave the 'Off connect' and the 'On connect' unchecked for a device, but keep 'Enable'. Then the mapping will do nothing for the device on connect, but you can still setup unpark and park mapping for that device.
* If you have settings for a device that you want to keep, but not use for the moment, you can just uncheck the enable checkbox. Your settings will stil be saved / savable.

![power-tab-setup](doc-media/5-powertab-mapping.jpg)

* As we added a power device we can now use the manual power switches on the Main Control tab.
* The status of these switches will always be updated when connecting, unparking and parking.
