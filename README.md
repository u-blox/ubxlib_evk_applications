# ubxlib cellular EVK applications for Raspberry PI and Windows

The purpose of this branch is to provide example EVK applications which run on the Raspberry PI or Windows. The application was originally from the [ubxlib-cellular-tracker-xplr-iot](https://github.com/u-blox/ubxlib_cellular_applications_xplr_iot) repository, but this repository is dedicated for building on the XPLR-IoT-1 Development platform. This repository is dedicated for Windows and Raspberry PI platforms.

As this application uses Cellular EVKs you can use any Combo Cellular+GNSS or Single Cellular + GNSS ADP boards (NEO-M8)

# Raspberry PI
Please set the `BUILD_TARGET_RASPBERRY_PI` #define in the application's [config.h](cellular_tracker/config/config.h) file.

## Building
Change to the application folder, [Cellular Tracker](cellular_tracker) for example, and type:  
`sudo cmake .`  
`sudo make`  

See `ubxlib/port/platform/linux` for further information about building on Raspberry PI.

## setTTY2EVK.sh
This is a bash script which will automatically create a symbol link to the correct ttyUSBx depending if there are two or four ttyUSBx found.  
ttyEVK -> ttyUSB0 (2 UARTs - old rev EVB)  
ttyEVK -> ttyUSB2 (4 UARTs - new rev EVB)  
  
Please run it with `sudo ./setTTY2EVK.sh` after you have connected the EVK to the Raspberry PI USB port.  
Please note, this is not work if you have other ttyUSB devices connected! Only one EVK is supported.

# Windows
Please set the `BUILD_TARGET_WINDOWS` #define in the application's [config.h](cellular_tracker/config/config.h) file.

## Building
Use the VisualStudio Code IDE and install the Micorsoft C++ compilers/dev kit. Install the CMake Tools extension.  
Right click on the CMakeLists.txt file and select build.  

See `ubxlib/port/platform/windows/mcu/win32` for further information about building on Windows.

# Applications

(Currently this repository has only one application).

* [Cellular Tracker](cellular_tracker).
  Publishes cellular signal strength parameters and location. Can be controlled to publish Cell Query results (+COPS=?)

# Application framework

The design of these applications is based around the same design.
Each application has access to the following functions:

## Application tasks
Each application is built from a number of `appTasks` which are either run in their own loop thread or their event executed separately.
Each `appTask` is based on the same 'boiler plate' design. Each application task has a `taskMutex`, `taskEventQueue` and `taskHandler` which are provided by the UBXLIB API.
The application `main()` function simply runs the `appTask's` loop or commands an `appTaks` to run individually at certain timing, or possibly other events. This makes up the application.

See [here](tasks) for further information of each `appTask` currently implemented.

## Application remote control

Each `appTask` has a `<IMEI>/<appTaskName>Control` topic where commands can be sent down to that `appTask`. Start, Stop task, measure 'now' etc.

A typical log output shows what the commands are:
> Subscribed to callback topic: 351457830026040/SensorControl
>
> With these commands:
>
> 1. MEASURE_NOW
> 2. START_TASK
> 3. STOP_TASK

## Application published data

Each appTask can publish their data/information to the MQTT broker to their own specific topic. This topic is well defined, being `<IMEI>/<appTaskName>`.

This data/information should normally be formatted as JSON.

## File Logging
Unlike the original XPLR Cellular Tracker application, this raspberry PI version does not log to a log file. Please use standard linux piping and systemd to run the application and view the output.

# Application main()
This is the starting point of the application. It will initialize the UBXLIB system and the device and run the application loop.

The Cellular settings, MQTT Profile and Security settings are set in the `config.txt` file. See here for an example: [config.txt](cellular_tracker/config.txt)

After the main system is configured it will `initialize` the application tasks. Here each task will create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are `initialized` the `Registration` and `MQTT` tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application will terminate if the INTERRUPT signal is provided to the application (`ctrl-c`), closing the MQTT broker connection and deregistering from the network. 
