#pragma once

#include "tapo_protocol.h"

#define TAPO_MAX_SEND_RETRIES 3
#define TAPO_MAX_RECONNECT_RETRIES 3


struct Command {
    String command;
    String expected_state;
    String response;
    int gotResponse;
    int send_retries; // number of sends left
    int reconnect_retries; // number of reconnects left
    unsigned long last_retry_time;
    int priority; // higher number = higher priority
};

class TapoDevice {
private:
    TapoProtocol protocol;
    Command currCommand;
    bool commandActive = false;
    unsigned int retryDelay = 1000;

    bool check_state(const String &expected_state) {
        String response = protocol.send_message("{\"method\":\"get_device_info\",\"params\":{}}");
        return response.indexOf(expected_state) != -1;
    }

    bool send_command(const String &command, const String &expected_state = "") {
        for (int i = 0; i < TAPO_MAX_RECONNECT_RETRIES; i++) {
            for (int j = 0; j < TAPO_MAX_SEND_RETRIES; j++) {
                if (expected_state == "" || check_state(expected_state)) {
                    return true;
                }
                protocol.send_message(command);
                //delay(1000); // Wait for the command to be applied
            }
            //Serial.println("TAPO_DEVICE: Failed to send command, rehandshaking...");
            protocol.rehandshake();
        }
        //Serial.println("TAPO_DEVICE: Failed to rehandshake, giving up command");
        
        return false;
    }

public:

    // 0 = no command, 1 = completed successfully, 2 = command in progress, 3 = failed
    int update() { // needs to be called in main loop to progress commands
        if (commandActive) {
            //Serial.print("TAPO_DEVICE: Command active: "); 
            //Serial.print(currCommand.command);
            //Serial.print(" Expected state: ");
            //Serial.print(currCommand.expected_state);
            //Serial.print("Response: ");
            //Serial.print(currCommand.response);
            //Serial.print("Send retries: ");
            //Serial.print(currCommand.send_retries);
            //Serial.print("Reconnect retries: ");
            //Serial.print(currCommand.reconnect_retries);
            //Serial.print("Last retry time: ");
            //Serial.print(currCommand.last_retry_time);
            //Serial.print("Priority: ");
            //Serial.println(currCommand.priority);

            if (millis() - currCommand.last_retry_time > retryDelay) {
                if (currCommand.send_retries > 0) { //try sending multiple requests
                    currCommand.send_retries--;
                    currCommand.response = protocol.send_message(currCommand.command);
                    currCommand.last_retry_time = millis();
                    if (currCommand.expected_state == "" || check_state(currCommand.expected_state)) { 
                        commandActive = false;
                        currCommand.gotResponse = 1;
                        return 1; // completed successfully
                    } else {
                        //Serial.println("TAPO_DEVICE: Command failed, retrying...");
                        return 2; // command in progress
                    }
                } else { // if all requests fail, try reconnecting
                    if (currCommand.reconnect_retries > 0) {
                        currCommand.reconnect_retries--;
                        //Serial.println("TAPO_DEVICE: Command failed, rehandshaking...");
                        protocol.rehandshake();
                        //Serial.println("TAPO_DEVICE: Command failed, rehandshaking done");
                        currCommand.last_retry_time = millis();
                        currCommand.send_retries = TAPO_MAX_SEND_RETRIES; // reset the number of send retries after reconnecting
                        return 2; // command in progress
                    } else {
                        //Serial.println("TAPO_DEVICE: Command failed, giving up...");
                        commandActive = false;
                        return 3; // failed
                    }
                }
            }
            else {
                return 2; // command in progress
            }
        } else {
            return 0; // no command
            //Serial.println("TAPO_DEVICE: Command inactive");
        }
    }

    void sendCommand(const String &command, const String &expected_state = "", int priority = 0) {
        //if (!commandActive) {
        //if (millis() - currCommand.last_retry_time > retryDelay) {
        if (priority > currCommand.priority || !commandActive) {
            currCommand.command = command;
            currCommand.expected_state = expected_state;
            currCommand.send_retries = TAPO_MAX_SEND_RETRIES;
            currCommand.reconnect_retries = TAPO_MAX_RECONNECT_RETRIES;
            currCommand.last_retry_time = millis();
            currCommand.priority = priority;
            currCommand.response = "";
            currCommand.gotResponse = 0;
            commandActive = true;
        //} else {
            //Serial.println("TAPO_DEVICE: Command not sent - not enough time has passed since last command attempt.");
        }
    }
    
    String getResponse() {
        return currCommand.response;
    }

    int getPriority() {
        return currCommand.priority;
    }

    int gotResponse() {
        return currCommand.gotResponse;
    }

    void begin(const String& ip_address, const String& username, const String& password) {
        protocol.handshake(ip_address, username, password);
    }

    void on() {
        String command = "{\"method\":\"set_device_info\",\"params\":{\"device_on\":true}}";
        String expected_state = "\"device_on\":true";
        sendCommand(command, expected_state,2);
        update();
        //send_command(command, expected_state);
    }

    void off() {
        String command = "{\"method\":\"set_device_info\",\"params\":{\"device_on\":false}}";
        String expected_state = "\"device_on\":false";
        sendCommand(command, expected_state,1);
        update();
        //send_command(command, expected_state);
    }

    String energy() {
        String command = "{\"method\":\"get_energy_usage\",\"params\":{}}";
        //Serial.println("TAPO_DEVICE: Sending energy command");
        String expected_state = "";
        sendCommand(command, expected_state,0);
        update();
        //Serial.println("TAPO_DEVICE: Energy command sent");
        return currCommand.response;
    }
    // String energy() {
    //     String command = "{\"method\":\"get_energy_usage\",\"params\":{}}";
    //     //String expected_state = "";
    //     for (int i = 0; i < TAPO_MAX_RECONNECT_RETRIES; i++) {
    //         for (int j = 0; j < TAPO_MAX_SEND_RETRIES; j++) {
    //             String response = protocol.send_message(command);
    //             delay(1000); // Wait for the command to be applied


    //             return response;
    //         }
    //         Serial.println("TAPO_DEVICE: Failed to send command, rehandshaking...");
    //         protocol.rehandshake();
    //     }
    //     Serial.println("TAPO_DEVICE: Failed to rehandshake, giving up command");
    //     return "";
    // }

    // String emeter() {
    //     String command = "{\"method\":\"get_emeter_usage\",\"params\":{}}";
    //     //String expected_state = "";
    //     for (int i = 0; i < TAPO_MAX_RECONNECT_RETRIES; i++) {
    //         for (int j = 0; j < TAPO_MAX_SEND_RETRIES; j++) {
    //             String response = protocol.send_message(command);
    //             delay(1000); // Wait for the command to be applied


    //             return response;
    //         }
    //         Serial.println("TAPO_DEVICE: Failed to send command, rehandshaking...");
    //         protocol.rehandshake();
    //     }
    //     Serial.println("TAPO_DEVICE: Failed to rehandshake, giving up command");
    //     return "";
    // }
};
