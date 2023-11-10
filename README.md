# BUILDING MINIMAL IMAGE FOR PEDESTRIAN SLAYER 2 USING YOCTO PROJECT
This documentation describes how a Yocto project is built on a Raspberry Pi 3 for Pedestrian Slayer 2. It covers from installing essential meta-layers for Raspberry Pi to some project oriented customizations like adding external meta-layers and libraries.

## Download Poky and Metadata Layers

```shell
git clone -b dunfell git://git.yoctoproject.org/poky.git
```

```shell
git clone -b dunfell git://git.yoctoproject.org/meta-raspberrypi.git
```
```shell
git clone -b dunfell git://git.openembedded.org/meta-openembedded
```

## Create Build Folder
```shell
source poky/oe-init-build-env
```
## Add Essential Layers
Add following meta-layers inside ```<YOCTO-PROJECT-PATH>/build``` with following commands:

```shell
bitbake-layers add-layer ../meta-openembedded/meta-oe
bitbake-layers add-layer ../meta-openembedded/meta-python
bitbake-layers add-layer ../meta-openembedded/meta-multimedia
bitbake-layers add-layer ../meta-openembedded/meta-networking
bitbake-layers add-layer ../meta-raspberrypi
bitbake-layers show-layers
```
## Set Essential Build Configs
* Make following modifications inside ```<YOCTO-PROJECT-PATH>/build/conf/local.conf``` to customize image for Raspberry Pi 3:

```bash
# target
MACHINE ?= "raspberrypi3-64"
...

# download folder
DL_DIR ?= "${HOME}/yocto-downloads"
...

# shared state folder
SSTATE_DIR ?= "${HOME}/yocto-sstate-cache"
...

# Dunfell (3.1.15), see more in <http://sstate.yoctoproject.org/>
SSTATE_MIRRORS ?= "file://.* http://sstate.yoctoproject.org/3.1.15/PATH;downloadfilename=PATH \n "

...

# add a feature
EXTRA_IMAGE_FEATURES_append = " ssh-server-dropbear"

# add editor
CORE_IMAGE_EXTRA_INSTALL_append = " nano"
...
```

### Adding Python Interpretter and Its Libraries
Add following lines in ```local.conf``` file:

```bash
...
IMAGE_INSTALL_append = " python3"
IMAGE_INSTALL_append = " python3-pip"
IMAGE_INSTALL_append = " python3-pyserial python3-numpy"
...
```
[source: https://developer.toradex.com/linux-bsp/application-development/programming-languages/python-in-linux/]

### Adding Make and GCC Compiler
Add following line in ```local.conf``` file:
```bash
...
EXTRA_IMAGE_FEATURES_append = " tools-sdk tools-debug"
...
```

### Adding GIT Support
Add following line in ```local.conf``` file:
```bash
...
IMAGE_INSTALL_append = " git"
...
```
[source: https://stackoverflow.com/questions/60624068/add-git-to-yocto-image]

### Adding systemd Support
Add following line in ```local.conf``` file:
```bash
...
DISTRO_FEATURES_append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"
VIRTUAL-RUNTIME_initscripts = ""
...
```
[source: https://www.instructables.com/Building-GNULinux-Distribution-for-Raspberry-Pi-Us/]

### Adding C++ Libraries
Add following line in ```local.conf``` file:
```bash
...
IMAGE_INSTALL_append = " libc-dev libstdc++-dev"
...
```

## Download Packages
```shell
bitbake core-image-base --runonly=fetch
```

## Add PREEMPT_RT Kernal Patch 

* Add following line for realtime support for Raspberry Pi in ```local.conf``` file:
```bash
...
PREFERRED_PROVIDER_virtual/kernel = "linux-raspberrypi-rt"
...
```

* After booting the target with new image, kernel version will be something like that:
```bash
Linux raspberrypi3 4.19.71-rt24-v7 #1 SMP PREEMPT RT Tue Dec 17 14:50:30 UTC 2019 armv7l armv7l armv7l GNU/Linux
```

## Build the Image
* Before building, make sure every configuration has been implemented. Check build configuration with following command:
```shell
bitbake core-image-base -n
```

Then, run following command on the terminal to build:

```shell
bitbake core-image-base
```

* The raw disk image is ```core-image-base-raspberrypi3-64.wic.bz2``` (If output name of image file is not manually altered) located in ```<YOCTO-PROJECT-PATH>/build/tmp/deploy/images/raspberrypi3-64/```.

* Use BelanEtcher to flash the image to SD Card or eMMC

[source: https://www.codeinsideout.com/blog/yocto/raspberry-pi/?utm_source=pocket_reader#download-poky-and-metadata-layers]

---
# ADDITIONAL CONFIGURATIONS

## Add WIFI and Bluetooth Support for RPI (Before Image Build)
* Add following lines in ```local.conf``` file to add wifi and bluetooth support:
```bash
...
IMAGE_INSTALL_append = " linux-firmware-bcm43430"
IMAGE_INSTALL_append = " i2c-tools bridge-utils hostapd dhcp-server iptables wpa-supplicant"
IMAGE_INSTALL_append = " bluez5"
...
```

* Following line should also be added in ```local.conf```:
```bash
...
DISTRO_FEATURES_append = " bluetooth wifi"
...
```

## Enable GPIO Support for RPI (Before Image Build)
* Add following line in ```local.conf``` file to add gpio support:
```bash
...
IMAGE_INSTALL_append = " rpi-gpio rpio pi-blaster"
...
```
[source: https://layers.openembedded.org/layerindex/branch/dunfell/layer/meta-raspberrypi/]

## Enable I2C Support for RPI
* Add following line in ```local.conf``` file to add I2C support:
```bash
...
ENABLE_I2C = "1"
KERNEL_MODULE_AUTOLOAD_rpi += " i2c-dev"
...
```

## Include Debian Package Manager
* Debian package manager is useful to instal additional software packages to the target system. ```apt-get``` or ```àpt``` commands can be used to download desired packages.
* Add example line in ```local.conf``` file to add debian package support :
```bash
...
PACKAGE_CLASSES += " package_deb"
...
EXTRA_IMAGE_FEATURES_append = " package-management"
...
```
[source: https://docs.yoctoproject.org/2.1/ref-manual/ref-manual.html#var-PACKAGE_CLASSES]

* To include ```apt``` command, add following line in  ```local.conf``` file:
```bash
...
IMAGE_INSTALL_append += " apt"
...
```

## CHANGE SPLASH LOGO (Easy Way - Before Image Build)
* Clone meta-splash layer with following command:
```shell
git clone https://github.com/hamzamac/meta-splash.git
```
* Replace the default ```logo.png``` image in ```meta-splash/recipes-core/psplash/files``` with your logo.

* Add meta layer in your yocto project
```shell
    bitbake-layers add-layer ../meta-splash
```
* Additional customizations for splash screen are available (changing color of loading bar etc.). 
[source: https://dev.to/makame/customize-boot-splash-screen-in-yocto-3bip]

## Setting Static IP
* It is possible to make an instruction that changes the content of ```/etc/network/interfaces``` of target image during rootfs install. To do this, it is necessary to create a custom meta-layer called ``meta-custom``` with running following command on the terminal:
 ```shell
 bitbake-layers create-layer ../meta-custom
 ```

* After create an example meta-layer create a directory called ```classes``` with following command:
```shell
mkdir -p ../meta-custom/classes
```

* Create a file named ```fixed-ip.bbclass``` located in ```~/meta-custom/classes`` with following command:
```shell
gedit ../meta-custom/classes/fixed-ip.bbclass
```

* Add following lines into the file (Note that change the network address and corresponding arguments in the file with desired ones):
```bash
#This function creates and appends the /etc/network/interfaces file to the rootfs
#
my_fixed_ip() {
cat <<EOF >eth0.network
iface eth0 inet static
    address 192.168.137.100
    netmask 255.255.255.0
    network 192.168.137.0
    gateway 192.168.137.1
EOF
cat eth0.network >> ${IMAGE_ROOTFS}/etc/network/interfaces
# Uncomment this line if systemd is used
#cp eth0.network  ${IMAGE_ROOTFS}/etc/systemd/network
rm eth0.network
}

ROOTFS_POSTINSTALL_COMMAND += " my_fixed_ip; "
```

* Save the file, and exit. Add ```meta-custom``` layer in the Yocto project with following command:
```shell
bitbake-layers add-layer ../meta-custom
```

* Then, add fixed-ip support in ```local.conf```:
```bash
...
USER_CLASSES_append = " fixed-ip"
...
```

[source: https://docs.windriver.com/bundle/Wind_River_Linux_Platform_Developers_Guide_LTS_21/page/cno1630363904950.html]

## Setting SSID for WLAN Connection
* Go to the following folder:
```shell
cd ../poky/meta/recipes-connectivity/wpa-supplicant/wpa-supplicant
```

* To edit content of the file, run following command:
```shell
gedit wpa_supplicant.conf-sane
```

* Add following lines inside of the file (Note that change the network address and corresponding arguments in the file with desired ones. Do not remove double quotes in ssid and psk):
```bash
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
        ssid="MYSSID"
        psk="MY-SECURE-PASSKEY"
        proto=RSN
        key_mgmt=WPA-PSK
}
```

* Save the file, and exit.

[source: https://lynxbee.com/how-to-enable-wifi-with-yocto-raspberry-pi3/#.ZDa18Y5Bwaw]

* Create a confguration file that its name is exactly <strong> wpa_supplicant.conf </strong> (remove ```.txt``` if it gets added).

### Connecting to unsecured networks:
* Change the content of the file with following lines:
```bash
country=US # Your 2-digit country code
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev # Include this line for Stretch
network={
    ssid="YOUR_NETWORK_NAME"
    key_mgmt=NONE
}
```
[source: https://howchoo.com/g/ndy1zte2yjn/how-to-set-up-wifi-on-your-raspberry-pi-without-ethernet]

## Change Thread Count for Build Process
If you suffer from out of RAM or need to decrease build time, it is advised to modify thread count for bitbake build process. Add following lines to ```local.conf```  file, then choose appropriate values for "x" and "y".

```bash
# Define thread count for build process
PARALLEL_MAKE = "-j x"
BB_NUMBER_THREADS = "y"
```
[source: https://stackoverflow.com/questions/66488550/how-to-add-more-threads-when-building]

## Clean Build
* To erase ```temp``` directory, run follow command on the terminal:
```shell
rm -rf build/tmp/
```

* To remove the sstate cache directory, run follow command on the terminal
```shell
rm -rf <YOCTO-PROJECT-PATH>/sstate-cache/
```
[source: https://tutorialadda.com/yocto/how-to-do-clean-build-in-yocto-project]
