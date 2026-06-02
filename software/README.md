# Software

This directory contains the higher-level applications and scripts for the ChimeraMultiFX project.

## Structure

- **`web-ui/`**: Contains the web application (HTML/JS/CSS, React, Vue, etc.) that acts as the GUI. This application is designed to be hosted by the ESP32 and accessed over WiFi.
- **`rpi-kiosk/`**: Contains scripts, OS configurations, and dependency lists to set up a raw Raspberry Pi to boot directly into a full-screen, locked-down browser pointing to the ESP32's web server.
