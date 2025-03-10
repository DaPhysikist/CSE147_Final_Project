from fastapi import FastAPI
from fastapi.responses import HTMLResponse, JSONResponse
import mysql.connector as mysql
from dotenv import load_dotenv
import os
from fastapi.staticfiles import StaticFiles
import uvicorn
import paho.mqtt.client as mqtt  # MQTT client
import json

load_dotenv()
db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']
db_name = "ShutEyeDataServer"

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_USER = os.environ['MQTT_USER']
MQTT_PASSWORD = os.environ['MQTT_PASSWORD']
MQTT_HISTORICAL_TOPIC = "shuteye/historical"
MQTT_PERIODIC_TOPIC = "shuteye/periodic"

# Database Connection Function
def get_db_connection():
    return mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)

# MQTT Callback Functions
def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT Broker with result code {rc}")
    client.subscribe(MQTT_HISTORICAL_TOPIC)
    client.subscribe(MQTT_PERIODIC_TOPIC)

def on_message(client, userdata, msg):
    print(f"Received message on topic {msg.topic}: {msg.payload.decode()}")

    try:
        data = json.loads(msg.payload.decode())
        appliance_name = data.get("appliance_name", "unknown")
        local_time = data.get("local_time", "0000-00-00 00:00:00")

        if msg.topic == MQTT_HISTORICAL_TOPIC:
            today_runtime = data.get("today_runtime", 0)
            month_runtime = data.get("month_runtime", 0)
            today_energy = data.get("today_energy", 0)
            month_energy = data.get("month_energy", 0)

            db = get_db_connection()
            cursor = db.cursor()
            query = """
                INSERT INTO ShutEyeDeviceEnergyDataHistorical
                (appliance_name, local_time, today_runtime, month_runtime, today_energy, month_energy)
                VALUES (%s, %s, %s, %s, %s, %s)
            """
            cursor.execute(query, (appliance_name, local_time, today_runtime, month_runtime, today_energy, month_energy))
            db.commit()
            db.close()
            print("Historical data inserted into MySQL.")

        elif msg.topic == MQTT_PERIODIC_TOPIC:
            current_power = data.get("current_power", 0)
            distance_ultrasonic = data.get("distance_ultrasonic", 0)
            distance_bluetooth = data.get("distance_bluetooth", 0)
            distance_ultrawideband = data.get("distance_ultrawideband", 0)
            user_presence_detected = data.get("user_presence_detected", 0)

            db = get_db_connection()
            cursor = db.cursor()
            query = """
                INSERT INTO ShutEyeDeviceEnergyDataPeriodicMeasurement
                (appliance_name, local_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected)
                VALUES (%s, %s, %s, %s, %s, %s, %s)
            """
            cursor.execute(query, (appliance_name, local_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected))
            db.commit()
            db.close()
            print("Periodic data inserted into MySQL.")

    except Exception as e:
        print(f"Error processing MQTT message: {e}")

mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

@app.get("/", response_class=HTMLResponse)       #returns the index HTML page for the default URL path
def get_html() -> HTMLResponse:                 
  with open('index.html') as html:             
    return HTMLResponse(content=html.read())

@app.get("/shuteye_historical_data/{appliance_name}", response_class=JSONResponse)
def fetch_historical_data(appliance_name: str):
    try:
        db = get_db_connection()
        cursor = db.cursor()
        query = "SELECT * FROM ShutEyeDeviceEnergyDataHistorical WHERE appliance_name = %s ORDER BY local_time DESC LIMIT 1;"
        cursor.execute(query, (appliance_name,))
        records = cursor.fetchall()

        response = {}
        for index, row in enumerate(records):
            response[index] = {
                "appliance_name": row[0],
                "local_time": row[1].strftime("%Y-%m-%d %H:%M:%S"),
                "today_runtime": row[2],
                "month_runtime": row[3],
                "today_energy": row[4],
                "month_energy": row[5]
            }

        db.close()
        return JSONResponse(content=response)

    except mysql.Error as err:
        return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
    except Exception as e:
        return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})

@app.get("/shuteye_periodic_measurement_data/{appliance_name}", response_class=JSONResponse)
def fetch_periodic_measurement_data(appliance_name: str):
    try:
        db = get_db_connection()
        cursor = db.cursor()
        query = "SELECT * FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = %s ORDER BY local_time ASC;"
        cursor.execute(query, (appliance_name,))
        records = cursor.fetchall()

        response = {}
        for index, row in enumerate(records):
            response[index] = {
                "appliance_name": row[0],
                "local_time": row[1].strftime("%Y-%m-%d %H:%M:%S"),
                "current_power": row[2],
                "distance_ultrasonic": row[3],
                "distance_bluetooth": row[4],
                "distance_ultrawideband": row[5],
                "user_presence_detected": row[6],
            }

        db.close()
        return JSONResponse(content=response)

    except mysql.Error as err:
        return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
    except Exception as e:
        return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})
    
@app.get("/appliance_names", response_class=JSONResponse)
def fetch_appliance_names() -> JSONResponse:
    try:
        db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
        cursor = db.cursor()
        query = "SELECT DISTINCT appliance_name FROM ShutEyeDeviceEnergyDataPeriodicMeasurement;"
        cursor.execute(query)
        appliance_names = cursor.fetchall()

        appliance_names = [name[0] for name in appliance_names]
        db.close()
        
        return JSONResponse(content={"appliance_names": appliance_names})
    except mysql.Error as err:
        return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
    except Exception as e:
        return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})
    
@app.get("/available_dates/{appliance_name}", response_class=JSONResponse)
def fetch_available_dates(appliance_name: str) -> JSONResponse:
    try:
        db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
        cursor = db.cursor()
        query = """
            SELECT DISTINCT DATE(local_time) 
            FROM ShutEyeDeviceEnergyDataPeriodicMeasurement 
            WHERE appliance_name = %s
            ORDER BY local_time DESC;
        """
        value = (appliance_name,)
        cursor.execute(query, value)
        dates = cursor.fetchall()

        # Extract date strings from the result
        dates = [date[0].strftime("%Y-%m-%d") for date in dates]
        db.close()
        
        return JSONResponse(content={"dates": dates})
    except mysql.Error as err:
        return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
    except Exception as e:
        return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=6543)
