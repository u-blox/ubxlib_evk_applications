/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * MQTT Task header
 *
 */
#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

#include "taskControl.h"

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t initMQTTTask(taskConfig_t *config);
int32_t startMQTTTaskLoop(commandParamsList_t *params);
int32_t stopMQTTTaskLoop(commandParamsList_t *params);
int32_t finalizeMQTTTask(void);

/* ----------------------------------------------------------------
 * TASK FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Publishes an MQTT message to a specified topic
/// @param pTopicName   The topic to publish the 
/// @param pMessage     The message string to publish
/// @param QoS          QoS value for the publishing to use
/// @param retain       A flag to indicate whether the message is to be retained
/// @return             Returns 0 on success, or negative on failure
int32_t publishMQTTMessage(const char *pTopicName, const char *pMessage, uMqttQos_t QoS, bool retain);

/// @brief Subscribe a callback function to a topic
/// @param taskTopicName    The topic to subscribe to
/// @param qos              QoS value for the publishing to use
/// @param callbacks        List of callbacks to be used for this subscription
/// @param numCallbacks     The number of callbacks in the list
/// @return             Returns 0 on success, or negative on failure
int32_t subscribeToTopicAsync(const char *taskTopicName, uMqttQos_t qos, callbackCommand_t *callbacks, int32_t numCallbacks);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    SEND_MQTT_MESSAGE,          // Sends a MQTT message
} mqttMsgType_t;

/// @brief MQTT message to send. Handles both MQTT and MQTT-SN topic name types
typedef struct SEND_MQTT_MESSAGE {
    union {
        char *pTopicName;
        uMqttSnTopicName_t *pShortName;
    } topic;

    char *pMessage;
    uMqttQos_t QoS;
    bool retain;
} sendMQTTMsg_t;

/// @brief Queue message structure for send any type of message to the MQTT application task
typedef struct MQTT_QUEUE_MESSAGE {
    mqttMsgType_t msgType;

    union {
        sendMQTTMsg_t message;
    } msg;
} mqttMsg_t;

#endif