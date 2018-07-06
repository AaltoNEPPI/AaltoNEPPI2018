Aalto NEPPI 2018
================

[Aalto NEPPI](https://neppi.aalto.fi) is an Aalto University course
where the students design and make a simple IoT thing from the ground
up, all the way to a functional or pre-prodution prototype.

This repository contains the source code for the Aalto NEPPI 2018
embeded software, running on the nRF52 based custom thing.  The
software is based on RIOT-OS and demonstrates how RIOT can be used to
make a Bluetooth Low Energy (BLE) GATT thing.

The thing communicates with the corresponding piece of host software,
running in Unity3D software under Linux, available at
[https://github.com/AaltoNEPPI/Unity_NEPPI_Skeleton](https://github.com/AaltoNEPPI/Unity_NEPPI_Skeleton).

Product specification
=====================

The 
[product specification](https://docs.google.com/document/d/1QEXLmvIUNTj_KuM_J-WDqp22YmqMVS499Dh2GHJ7nng), 
including the software architecture, is available at Google Docs.

Building instructions
=====================

```
   $ git clone --recurse-submodules git@github.com:AaltoNEPPI/AaltoNEPPI2018.git
   $ cd AaltoNEPPI2018
   $ make
   $ make flash
   $ make term
```

Testing instructions
====================

TBD
