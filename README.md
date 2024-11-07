About/Use
===============
This code aim's to read the tine and object data from the PLC, and run the object detection algorithm to write the computed boolean array back onto the PLC
This uses Beckhoff's ADS library


Compile & usage
===============
```shell
# clone the repository
git clone https://github.com/camron-BHF/adsCppWritter.git
# change into root of the cloned repository
cd adsCppWritter
# configure meson to build the library into "build" dir
meson setup build
# let ninja build the library
ninja -C build
```

Prepare your client to run the example/example.cpp
==================================================
- set "remoteNetId" to the AMS NetId of your TwinCAT target (this is the AMS NetId found in the "About TwinCAT System window" e.g. "192.168.0.2.1.1").
- set "remoteIpV4" to the IP Address of your TwinCAT target (e.g. 192.168.0.2)
- (optional) enable bhf::ads::SetLocalAddress() and set to the AMS NetId you choose for the ADS client (e.g. 192.168.0.1.1.1).

```shell
# configure meson to build example into "build" dir
meson example/build example
# let ninja build the example
ninja -C example/build
# and run the example
./example/build/example
```
---

