# Camper-Logger

This is the firmware for an ESP32-based data logger for campers.

It can be used to gather some data (see below) and upload it to some form of data storage.
You can either use an external host (if you have connectivity in your camper), or a local
host (like a Raspberry Pi) if you don't. The logger itself does not store any data!

## Features

- Read GPS coordinates, speed, heading and time
- Read solar statistics from a Victron MPPT charge regulator
- Measure temperatures using Dallas 1-Wire sensors
- Arduino OTA via IDE
- Over the air software updates (OTA)
- Periodically upload this info to the server
- Data upload is done over https
- Supports logging directly to InfluxDB2.0 (including https support)
- Configurable via webinterface
- Output of all measurements in JSON via http for local displaying



