# BUILDING MINIMAL IMAGE FOR PEDESTRIAN SLAYER 2 USING YOCTO PROJECT
This documentaton describes how a yocto project is built on Raspberry Pi 3 for Pedestrian Slayer 2. Documentation covers from installing requred meta layers essential for building Yocto project and some customisations like installing external meta layers and libraries to the target system.

## DOWNLOAD POKY AND METADATA LAYERS

```shell
git clone -b dunfell git://git.yoctoproject.org/poky.git
```

```shell
git clone -b dunfell git://git.yoctoproject.org/meta-raspberrypi.git
```
```shell
git clone -b dunfell git://git.openembedded.org/meta-openembedded
```

## CREATE BUILD FOLDER
```shell
source poky/oe-init-build-env
```
## ADD ESSENTIAL LAYERS
Add following meta layers inside ```~/build``` with following commands:

```shell
bitbake-layers add-layer ../meta-openembedded/meta-oe
bitbake-layers add-layer ../meta-openembedded/meta-python
bitbake-layers add-layer ../meta-openembedded/meta-multimedia
bitbake-layers add-layer ../meta-openembedded/meta-networking
bitbake-layers add-layer ../meta-raspberrypi
bitbake-layers show-layers
```
## SET BUILD CONFIGS
```~/build/conf/local.conf```

```
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

```
...
IMAGE_INSTALL_append = " python3"
IMAGE_INSTALL_append = " python3-pip"
IMAGE_INSTALL_append = " python3-pyserial python3-numpy"
...
```
[source: https://developer.toradex.com/linux-bsp/application-development/programming-languages/python-in-linux/]

### Adding Make and GCC Compiler
Add following line in ```local.conf``` file:
```
...
EXTRA_IMAGE_FEATURES_append = " tools-sdk tools-debug"
...
```
### Adding SQLite
Add following line in ```local.conf``` file:
```
...
IMAGE_INSTALL_append = " sqlite3"
...
```
[Source: https://stackoverflow.com/questions/40017676/how-to-add-libsqlite3-dev-to-yocto-kernel-for-sqlite3-c-program]


### Adding GIT Support
Add following line in ```local.conf``` file:
```
...
IMAGE_INSTALL_append = " git"
...
```
[source: https://stackoverflow.com/questions/60624068/add-git-to-yocto-image]

### Adding systemd Support
Add following line in ```local.conf``` file:
```
...
DISTRO_FEATURES_append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"
VIRTUAL-RUNTIME_initscripts = ""
...
```
[source: https://www.instructables.com/Building-GNULinux-Distribution-for-Raspberry-Pi-Us/]

### Adding ROS2 Meta-Layer (Not implemented)
* Download meta-ros layer (for dunfell branch)
```shell
git clone -b dunfell https://github.com/ros/meta-ros.git
```

* Add followings in end of ```~/build/conf/bblayers.conf```
```
# define the ROS 2 Yocto target release
ROS_OE_RELEASE_SERIES = "dunfell"

# define ROS 2 distro
ROS_DISTRO = "foxy"
```

* To add meta-layers into the project run following commands:
```shell
bitbake-layers add-layer ../meta-ros/meta-ros-backports-hardknott
bitbake-layers add-layer ../meta-ros/meta-ros-backports-gatesgarth
bitbake-layers add-layer ../meta-ros/meta-ros-common
bitbake-layers add-layer ../meta-ros/meta-ros2
bitbake-layers add-layer ../meta-ros/meta-ros2-foxy
bitbake-layers show-layers
```

* New layers will be shown on  ```~/build/conf/bblayers.conf```.

* Run following command on the terminal:
```shell
bitbake -p ros-core
```

[source: https://github.com/ros/meta-ros/wiki/OpenEmbedded-Build-Instructions]

### Adding ROS2 Libraries (Not implemented)
* Add following line in ```local.conf``` file:
```
...
IMAGE_INSTALL_append = " ros2launch"
IMAGE_INSTALL_append = " screen"
IMAGE_INSTALL_append = " libc-dev libstdc++-dev"
IMAGE_INSTALL_append = " libc6-dev"
...
```

## DOWNLOAD PACKAGES
```shell
bitbake core-image-base --runonly=fetch
```

## ADD PREEMPT_RT KERNAL PATCH 

* Add following line for realtime support for Raspberry Pi in ```local.conf``` file:
```
...
PREFERRED_PROVIDER_virtual/kernel = "linux-raspberrypi-rt"
...
```

* After booting the target with new image, kernel version will be something like that:
```
Linux raspberrypi3 4.19.71-rt24-v7 #1 SMP PREEMPT RT Tue Dec 17 14:50:30 UTC 2019 armv7l armv7l armv7l GNU/Linux
```

## BUILD THE IMAGE
* Before building, make sure every configuration has been implemented. Check build configuration with following command:
```shell
bitbake core-image-base -n
```

Then, run following command on the terminal to build:

```shell
bitbake core-image-base
```

* The raw disk image is ```core-image-base-raspberrypi3-64.wic.bz2``` (If output name of image file is not manually altered) located in ```~/build/tmp/deploy/images/raspberrypi3-64/```.

* Use BelanEtcher to flash the image to SD Card or eMMC


[source: https://www.codeinsideout.com/blog/yocto/raspberry-pi/?utm_source=pocket_reader#download-poky-and-metadata-layers]

---
# CONFIGURING YOCTO PROJECT

## DISABLING ROOT SSH LOGIN OVER NETWORK (Before Image Build)

* Go to the path: ```poky/meta/recipes-core/dropbear/dropbear```.  The ```init``` file is presented in dropbear folder.

* Add ```"-g -s"``` option in DROPBEAR_EXTRA_ARGS. The result is as follows:

```
...
DROPBEAR_EXTRA_ARGS="-g -s"
...
```

Note:

-w:  Disallow root logins

-s:  Disable password logins

-g:  Disable password logins for root

[source: http://embeddedguruji.blogspot.com/2019/02/disabling-root-ssh-login-over-network.html]


## ADD WIFI AND BLUETOOTH SUPPORT FOR RPI (Before Image Build)
* Add following lines in ```local.conf``` file to add wifi and bluetooth support:

```
...
IMAGE_INSTALL_append = " linux-firmware-bcm43430"
IMAGE_INSTALL_append = " i2c-tools bridge-utils hostapd dhcp-server iptables wpa-supplicant"
IMAGE_INSTALL_append = " bluez5"
...
```

* Following line should also be added in ```local.conf```:
```
...
DISTRO_FEATURES_append = " bluetooth wifi"
...
```

## ENABLE GPIO SUPPORT FOR RPI (Before Image Build)
* Add following line in ```local.conf``` file to add gpio support:

```
...
IMAGE_INSTALL_append = " rpi-gpio rpio pi-blaster"
...
```
[source: https://layers.openembedded.org/layerindex/branch/dunfell/layer/meta-raspberrypi/]

## ENABLE I2C SUPPORT FOR RPI (Before Image Build)
* Add following line in ```local.conf``` file to add i2c support:

```
...
ENABLE_I2C = "1"
KERNEL_MODULE_AUTOLOAD_rpi += " i2c-dev"
...
```

## INCLUDE APT AND APT-GET PACKAGE MANAGERS (Before Image Build)
* Add example line in ```local.conf``` file to rename the output image:
```
...
PACKAGE_CLASSES += " package_deb"
...
EXTRA_IMAGE_FEATURES_append = " package-management"
...
```
[source: https://docs.yoctoproject.org/2.1/ref-manual/ref-manual.html#var-PACKAGE_CLASSES]


## SET NAME OF OUTPUT IMAGE (Before Image Build)
* Add example line in ```local.conf``` file to rename the output image:
```
...
IMAGE_NAME = "${IMAGE_BASENAME}-${MACHINE}-${DATETIME}"
...
```
[source: https://www.codeinsideout.com/blog/yocto/concepts/#bitbake]

Note that adding timestamp may results in following error in multiple times:

<span style="color: red" >When reparsing xxx, the basehash value changed from yyy to zzz. The metadata is not deterministic and this needs to be fixed.</span>


## CHANGE SPLASH LOGO (Before Image Build)
In order to change splash logo, modify the content of ```psplash-poky-img.h``` located in ```/poky/meta-poky/recipes-core/psplash```.
[source: https://archive.fosdem.org/2018/schedule/event/rt_linux_with_yocto/attachments/slides/2684/export/events/attachments/rt_linux_with_yocto/slides/2684/Yocto_RT.pdf]

## CHANGE SPLASH LOGO (Easy Way - Before Image Build)
* Clone meta-splash layer
```
git clone https://github.com/hamzamac/meta-splash.git
```
Replace the default ```logo.png``` image in ```meta-splash/recipes-core/psplash/files``` with your logo image with name logo.png
* Add meta layer in your yocto project
```
    bitbake-layers add-layer ../meta-splash
```
 
[source: https://dev.to/makame/customize-boot-splash-screen-in-yocto-3bip]


## SETTING STATIC IP (Before Image Build)
* It is possible to make an instruction that changes the content of ```/etc/network/interfaces``` of target image during rootfs install. To do this, it is necessary to create a custom meta-layer called ``meta-custom``` with running following command on the terminal:
 ```shell
 bitbake-layers create-layer ../meta-custom
 ```

* After create an example meta-layer create a directory called ```classes``` with following command:
```shell
mkdir -p ~/meta-custom/classes
```

* Create a file named ```fixed-ip.bbclass``` located in ```~/meta-custom/classes`` with following command:
```shell
gedit ~/meta-custom/classes/fixed-ip.bbclass
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
```
...
USER_CLASSES_append = " fixed-ip"
...
```

[source: https://docs.windriver.com/bundle/Wind_River_Linux_Platform_Developers_Guide_LTS_21/page/cno1630363904950.html]

## SETTING STATIC IP (After Image Build)
* Go to following file
~~~shell
$ nano /etc/network/interfaces (Not implemented)
~~~

* Add following lines:

```
iface eth0 inet static
    address 192.168.137.100
    netmask 255.255.255.0
    network 192.168.137.0
    gateway 192.168.137.1
```
```local.conf``` re Image Build)```.

* Create a file named ```store-ssid.bbclass``` located in ```~/meta-custom/classes`` with following command:
```shell
gedit ~/meta-custom/classes/store-ssid.bbclass
```

* Add following lines into the file (Note that change the network address and corresponding arguments in the file with desired ones. Do not remove double quotes in ssid and psk):
```bash
#This function creates and appends the /etc/wpa_supplicant.conf file to the rootfs
#
store_ssid() {
cat <<EOF >wpa_supplicant
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=1

network={
    ssid="YOUR_NETWORK_NAME"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
}
EOF
cat wpa_supplicant.conf > ${IMAGE_ROOTFS}/etc/wpa_supplicant.conf
rm wpa_supplicant.conf
}

ROOTFS_POSTINSTALL_COMMAND += " store_ssid"
```

* Add meta-layer into the project if it does not included in previous procedure.

* Then, add store-ssid support in ```local.conf```:
```
...
USER_CLASSES_append = " store-ssid"
...
```

## SETTING SSID FOR WLAN CONNECTION [SECOND WAY] (Before Image Build)

* Go to the following folder:
```shell
cd ~/poky/meta/recipes-connectivity/wpa-supplicant/wpa-supplicant
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

## SETTING SSID FOR WLAN CONNECTION (After Image Build)

* Open a text editor to create a new file and following lines(Do not remove double quotes in ssid and psk):
```bash (Not implemented)
    ssid="YOUR_NETWORK_NAME"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
}
```

* Create a confguration file that its name is exactly <strong> wpa_supplicant.conf </strong> (remove ```.txt``` if it gets added).

### Connecting to unsecured networks:
* Change the content of the file with following lines:
```
country=US # Your 2-digit country code
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev # Include this line for Stretch
network={
    ssid="YOUR_NETWORK_NAME"
    key_mgmt=NONE
}
```

[source: https://howchoo.com/g/ndy1zte2yjn/how-to-set-up-wifi-on-your-raspberry-pi-without-ethernet]

## IMAGE SANITY CHECKS FOR ROS (After Image Boot) (Not implemented)
### ROS 1 Sanity Check:
Run following commands in target terminal:
```
ping lge.com
```
Skip if built with "ros-implicit-workspace" in IMAGE_FEATURES.
```
source /opt/ros/melodic/setup.sh
```
Start roscore and check output.
```
roscore &
```

### ROS 2 Sanity Check:
Run following commands in target terminal:
```
ping lge.com
```
Skip if built with "ros-implicit-workspace" in IMAGE_FEATURES.
```
source ros_setup.sh
```
```
echo $LD_LIBRARY_PATH
```
```
ros2 topic list
```
[source: https://github.com/ros/meta-ros/wiki/OpenEmbedded-Build-Instructions]

## CHANGE THREAD COUNT FOR BUILD PROCESS
If you suffer from out of RAM or need to decrease build time, it is advised to modify thread count for bitbake build process. Add following lines to ```local.conf```  file, then choose appropriate values for "x" and "y".

```
# Define thread count for build process
PARALLEL_MAKE = "-j x"
BB_NUMBER_THREADS = "y"
```
[source: https://stackoverflow.com/questions/66488550/how-to-add-more-threads-when-building]

## CLEAN BUILD
* To erase ```temp``` directory, run follow command on the terminal:
```
rm -rf build/tmp/
```

* To remove the sstate cache directory, run follow command on the terminal
```
rm -rf <YOCTO-PROJECT-PATH>/sstate-cache/
```
[source: https://tutorialadda.com/yocto/how-to-do-clean-build-in-yocto-project]
