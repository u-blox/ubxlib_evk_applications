# ubxlib cellular applications for Raspberry PI

The purpose of this branch is to provide example applications which run on the Raspberry PI. The application was originally from the [ubxlib-cellular-tracker-xplr-iot](https://github.com/u-blox/ubxlib_cellular_applications_xplr_iot) repository, but this branch is for building and running on a Raspberry PI.

Because this branch is for the Raspberry PI you can use any combo Cellular+GNSS module that ubxlib supports. Currently this application does NOT support external GNSS devices.

# Applications

(Currently this repository has only one application).

* [Cellular Tracker](applications/cellular_tracker).
  Publishes cellular signal strength parameters and location. Can be controlled to publish Cell Query results (+COPS=?)

* [...]()

# Application framework

The design of these applications is based around the same design.
Each application has access to the following functions:

## Application tasks

Each application is built from a number of `appTasks` which are either run in their own loop thread or their event executed separately.

Each `appTask` is based on the same 'boiler plate' design. Each application task has a `taskMutex`, `taskEventQueue` and `taskHandler` which are provided by the UBXLIB API.

The application `main()` function simply runs the `appTask's` loop or commands an `appTaks` to run individually at certain timing, or possibly other events. This makes up the application.

See [here](applications/tasks) for further information of each `appTask` currently implemented.

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
This is the starting point of the application. It will initialize the UBXLIB system and the device.

If present on the file system the application will load the MQTT credentials. If the MQTT credentials are not stored on the file system, it can be specified using a `#define` in the `config.h` file.

After the main system is initialized it will initialize the application tasks. Here each task will create a Mutex and eventQueue. These can be used to know if the appTask is running something, and communicate a command to it.

Once all the application tasks are initialized the Registration and MQTT application tasks will `start()`. Here they will run their task loop, looking after the registration and MQTT broker connection.

The main application will terminate if the INTERRUPT signal is provided to the application (`ctrl-c`), closing the MQTT broker connection, deregistering from the network. 
