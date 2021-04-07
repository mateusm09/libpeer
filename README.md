# Pear - WebRTC Toolkit for IoT/Embedded Device

![pear-ci](https://github.com/sepfy/pear/actions/workflows/pear-ci.yml/badge.svg)

Pear is a WebRTC SDK written in C. The SDK aims to integrate IoT/Embedded device with WebRTC applications.

### Dependencies

* [libsrtp](https://github.com/cisco/libsrtp)
* [libnice](https://github.com/libnice/libnice)
* [librtp](https://github.com/ireader/media-server)


### Getting Started

```
# sudo apt -y install libglib2.0-dev libssl-dev git cmake ninja-build
# sudo pip3 install meson
# git clone --recursive https://github.com/sepfy/pear
# ./build-third-party.sh
# mkdir cmake
# cd cmake
# cmake ..
# make
```

### Examples
This example is tested on Raspberry Pi zero W with image 2021-01-11-raspios-buster-armhf-lite.img

1. Open index.html with browser in your computer
2. Copy textarea content to remote_sdp.txt
3. Download test [video file](http://www.live555.com/liveMedia/public/264/test.264)
4. Run local file sample. (Please ensure that remote_sdp.txt and test.264 in the same directory)
```
./cmake/examples/local_file/local_file
```
6. Copy base64 sdp answer to browser.
7. Click start session.

You will see the video on your browser.
