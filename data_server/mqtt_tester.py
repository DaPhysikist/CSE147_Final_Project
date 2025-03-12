import paho.mqtt.client as mqtt
import json

# MQTT Configuration
MQTT_BROKER = "192.168.8.27"
MQTT_PORT = 1883  # Change if needed
MQTT_TOPIC = "espresense/devices/pranav-phone/microwave"
MQTT_USERNAME = "DaUser"
MQTT_PASSWORD = "DaPassword123!"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        distance = payload.get("distance", "Unknown")
        print(f"Received distance data from {msg.topic}: {distance} meters")
    except json.JSONDecodeError:
        print(f"Error decoding JSON from topic {msg.topic}")

def main():
    client = mqtt.Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_forever()
    except Exception as e:
        print(f"MQTT connection error: {e}")

if __name__ == "__main__":
    main()
