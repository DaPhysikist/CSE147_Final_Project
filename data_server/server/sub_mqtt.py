import paho.mqtt.client as mqtt
import json
from dotenv import load_dotenv
import os

load_dotenv()

# MQTT Configuration
MQTT_BROKER = "localhost"
MQTT_PORT = 1883  # Change if needed
MQTT_TOPIC = "espresense/rooms/+/telemetry"  # Modify based on your topic structure
MQTT_USER = os.environ['MQTT_USER']
MQTT_PASSWORD = os.environ['MQTT_PASSWORD']

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        print(f"Received data from topic {msg.topic}:")
        print(json.dumps(payload, indent=4))
    except json.JSONDecodeError:
        print(f"Error decoding JSON from topic {msg.topic}")

def main():
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_forever()
    except Exception as e:
        print(f"MQTT connection error: {e}")

if __name__ == "__main__":
    main()