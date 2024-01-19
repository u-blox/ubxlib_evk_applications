# Cellular Tracking Application
The Cellular tracking application purpose is to periodically monitor the signal quality of the cellular environment, its location and scan the visible cellular base stations if the START_CELL_SCAN message is sent via MQTT to the device.

All collected information is sent to the cloud via the embedded MQTT/MQTT-SN client to an MQTT Broker.

Once turned ON, by default the application monitors the cellular signal quality. Once there is a GNSS fix, the location is also published to the cloud. If the START_CELL_SCAN message is received a base station scan is initialized.

## Configuring the building environment
The Raspberry PI is used to compile the application.

clone this repository on to the raspberry PI:
- git clone https://github.com/u-blox/ubxlib_cellular_applications_xplr_iot/tree/raspberryPi

<br>You will need to install cmake, libssl-dev and libgpiod-dev 
- sudo apt-get install cmake
- sudo apt-get install libssl-dev
- sudo apt-get install libsgpiod-dev

To configure what cellular module and what UART port the module is on, you must set the U_FLAGS environment variable.
- export U_FLAGS='-DU_CFG_APP_CELL_UART=0 -DU_CFG_TEST_CELL_MODULE_TYPE=U_CELL_MODULE_TYPE_SARA_R422

## Compiling the application
The build system uses cmake and make to configure and compile the application. `cmake` will use the environment variable to configure the build.

1. Change the directory to the `application\cellular_tracker` folder
2. sudo -E cmake .
3. sudo make

## Configuring the application
Using the [config.h](config/config.h) file in the [config](config/) folder you will find the basic application configuration settings, like the level of debug

# LED indicators
There are no LED indicators on the Raspberry PI for this project.

# Application remote commands

The application can be remotely controlled through various topics which are subscribed to by the application tasks. 

A typical log output shows what the commands are for each task, and main application
> Subscribed to callback topic: 351457830026040/SensorControl
>
> With these commands:
>
> 1. MEASURE_NOW
> 2. START_TASK
> 3. STOP_TASK


## <IMEI\>\AppControl

### SET_DWELL_TIME <milliseconds\>
Sets the period between the main loop performing the location and signal quality measurements. Default is 5 seconds.

### SET_LOG_LEVEL <log level\>
Sets the logging level of the application. Default is '2' for INFO log level.

    0: TRACE
    1: DEBUG
    2: INFO
    3: WARN
    4: ERROR
    5: FATAL

It could be possible to increase the logging of an application remotely by changing the logging value from '2' to '1'.

## <IMEI\>\CellScanControl

### START_CELL_SCAN
Starts a cell scan process, just as if you had pressed Button #2

# NOTES
## Thingstream SIMS
Thingstream SIMs can be used with two APNS; TSUDP or TSIOT.

TSUDP is ONLY for MQTT-Anywhere service (using MQTT-SN), and does not allow any other internet traffic. This means when using TSUDP the NTP date/time request is not performed. This 'TSUDP' APN is listed as a 'restricted' APN in the [tasks/registrationTask.h](../tasks/registrationTask.h) file.

TSIOT can be used for normal internet services and as such should be used when using other MQTT brokers, or even other MQTT-SN gateways.