dsrl-ios-control
================

Control DSRL Camera connect to pc (via USB) using ipad or iphone. See live preview, take snapshot and more

### Description

This project is based on two part:
- A server side running on a pc (linux or mac) connectd thought usb to dsrl camera; the server stream camera live view ang get controll over the network;
- An ios client side on iphone or ipad. On mobile side is possible see the live view of the camera, controll and take snapshot.
. 

### Structure

libgphoto2 is used on server side to controll and capture live view of camera. mjpeg stream is send using udp socket to ios device and udp is also used to controll the camera.

### Current Status

Very early stage, just a spaghetti code test but working as test.
Can see live view and take snapshot.
Tested with Ubuntu machine as server and Nikon D90.



