###### pretty screenies here

# OpenFIRE App
### Reference configuration utility for the [OpenFIRE light gun system](https://github.com/TeamOpenFIRE/OpenFIRE-Firmware), written in Qt & C++.

## Features:
 - Cross-platform QT application, portable across desktops to Pis and other OSes.
 - Simple to use: select the gun from the dropdown, and configure away!
 - See and manage current pins layout, toggle on and off custom mappings, set other tunables, and change the gun's USB identifier (with built-in decimal-to-hex conversion for your convenience!).
 - Also serves as a testing utility for button input and solenoid/rumble force feedback.

## Running:
Boards flashed with OpenFIRE *must be plugged in **before** launching the application.* The app will notify if it can't find any compatible boards connected.

### For Linux:
##### Requirements: Anything with QT5 support.
 - Arch Linux: TODO: Aur PKGBUILD when public
 - Other distros: Try the latest binary (built for Ubuntu 20.04 LTS, but should work for most distros?)
 - Make sure your user is part of the `dialout` group (`# usermod -a -G dialout insertusernamehere`)

### For Windows:
##### Requirements: Windows 7 and up (64-bit only).
 - Download the latest release zip.
 - Extract the `OpenFIREapp` folder from the archive to anywhere that's most convenient on your system - `OpenFIREapp.exe` should be sitting next to `Qt5Core.dll` and others, as well as the `platforms` and `styles` folders.
 - Start `OpenFIREapp.exe`

## Building:
### For Linux:
#### Requires `qt-base`, `qt-serialport`, `qt-svg`
 - Clone the repo:
   ```
   git clone https://github.com/TeamOpenFIRE/OpenFIRE-App
   ```
 - Setup build directory:
   ```
   cd OpenFIRE-App
   mkdir build && mkdir build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```
 - Make:
   ```
   make
   ```
 - And run:
   ```
   ./OpenFIREapp
   ```
### For Windows:
 - Should be buildable through CMake or the QT Creator IDE.
###### TODO: any specific instructions for this? Seong doesn't use Windows.

### TODO:
 - Implement version comparison to latest OpenFIRE GitHub release (or at least latest as of the GUI version).
 - Add one-click firmware installation/updating from GUI (for both already flashed guns AND RP2040 devices in bootloader mode).
 - Add radio buttons for preset TinyUSB Identifier settings (for the extra picky distros that depend on set PIDs or names...)
 - Add icon, logo.
 - Layouts code should be prettier, though Qt really encourages wanton destruction of objects just to clear pages.

### Special Thanks:
 - Samuel Ballentyne, Prow7, and co. for their work on the SAMCO system that lead to the creation of GUN4ALL & OpenFIRE.
 - ArcadeForums posters that voiced their thoughts and suggestions for the GUN4ALL project.
 - GUN4ALL testers and everyone that provided feedback to GUN4ALL.
