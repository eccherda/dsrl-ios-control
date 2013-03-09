Server side
===========


Running on pc with Linux or Mac. Control camera using libgphoto2 and stream data to ios device


### Linux

Tested on Ubuntu 12-04,
Install gphoto


    sudo apt-get install build-essential
    sudo apt-get install gphoto2
    sudo apt-get install libgphoto2-2
    sudo apt-get install libgphoto2-2-dev

 
To build use:

    ./build.sh   (..I will make Makefile...)

Actualy in necessary increase default udp buffer dimension on system

	sudo sysctl -w net.core.wmem_max=1048576
	sudo sysctl -w net.core.rmem_max=1048576

You can make this permanent by adding the following lines to /etc/sysctl.conf:

	net.core.wmem_max=1048576
	net.core.rmem_max=1048576


to start:
- connect camera (unmount if Ubuntu mount automaticaly)
- open app in ios device and see id address
- start with: ./dsrl-stream 192.168.1.xx 5600

For problem with camera try if is working

	gphoto --auto-detect
	gphoto --capture-image

### MacOS X10

Install Macports (http://www.macports.org/install.php)

Install gphoto

	sudo port install gphoto2
	sudo port install libgphoto2

 
To build use:

    ./build-mac.sh   (..I will make Makefile...)




to start:
- connect camera (unmount if Ubuntu mount automaticaly)
- stop PTPCamera

	killall PTPCamera

- open app in ios device and see id address
- start with: ./dsrl-stream 192.168.1.xx 5600

For problem with camera try if is working

	gphoto --auto-detect
	gphoto --capture-image