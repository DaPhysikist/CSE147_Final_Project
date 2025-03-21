from fastapi import FastAPI, Request, Form
from fastapi.responses import Response
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
import uvicorn
import os
import mysql.connector as mysql
from dotenv import load_dotenv

load_dotenv()                 # Retreives the credentials needed to conenct to the data server
db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']
db_name = "ShutEyeDataServer"                           

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

@app.get("/", response_class=HTMLResponse)       # Returns the index HTML page for the default URL path
def get_html() -> HTMLResponse:                 
  with open('index.html') as html:             
    return HTMLResponse(content=html.read())

@app.get("/shuteye_historical_data/{appliance_name}", response_class=JSONResponse)   # REST API route to fetch historical data
def fetch_historical_data(appliance_name: str) -> JSONResponse:
    try:
        db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
        cursor = db.cursor()
        query = "SELECT * FROM ShutEyeDeviceEnergyDataHistorical WHERE appliance_name = %s ORDER BY local_time DESC LIMIT 1;"
        value = (appliance_name,)
        cursor.execute(query, value)
        records = cursor.fetchall()
        response = {}
        for index, row in enumerate(records):  # Iterate through the database data to construct the dict to return as JSON
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
    

@app.get("/shuteye_periodic_measurement_data/{appliance_name}", response_class=JSONResponse) # REST API route to fetch periodic measurement data
def fetch_periodic_measurement_data(appliance_name: str) -> JSONResponse:
    try:
        db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
        cursor = db.cursor()
        query = "SELECT * FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = %s ORDER BY local_time ASC;"
        value = (appliance_name,)
        cursor.execute(query, value)
        records = cursor.fetchall()
        response = {}
        for index, row in enumerate(records):  # Iterate through the database data to construct the dict to return as JSON
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
    
@app.post("/shuteye_historical_data")      # REST API route to insert historical data into the database
def insert_historical_data(appliance_historical_data: dict):
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  query = "insert into ShutEyeDeviceEnergyDataHistorical(appliance_name, local_time, today_runtime, month_runtime, today_energy, month_energy) values (%s, %s, %s, %s, %s, %s)"
  value = (appliance_historical_data['appliance_name'], appliance_historical_data['local_time'], appliance_historical_data['today_runtime'], appliance_historical_data['month_runtime'], appliance_historical_data['today_energy'], appliance_historical_data['month_energy'])
  try:
    cursor.execute(query, value)
  except mysql.Error as err:
      return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
  except Exception as e:
      return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})
  db.commit()
  db.close()

@app.post("/shuteye_periodic_measurement_data")    # REST API route to insert periodic measurement data into the database
def insert_periodic_measurement_data(appliance_periodic_data: dict):
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  query = "insert into ShutEyeDeviceEnergyDataPeriodicMeasurement(appliance_name, local_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected) values (%s, %s, %s, %s, %s, %s, %s)"
  value = (appliance_periodic_data['appliance_name'], appliance_periodic_data['local_time'], appliance_periodic_data['current_power'], appliance_periodic_data['distance_ultrasonic'], appliance_periodic_data['distance_bluetooth'], appliance_periodic_data['distance_ultrawideband'], appliance_periodic_data['user_presence_detected'])
  try:
    cursor.execute(query, value)
  except mysql.Error as err:
      return JSONResponse(status_code=500, content={"error": f"Database error: {err}"})
  except Exception as e:
      return JSONResponse(status_code=500, content={"error": f"An error occurred: {e}"})
  db.commit()
  db.close()

@app.get("/appliance_names", response_class=JSONResponse)    # Retrieves list of unique appliance names from the database for the web and mobile app dropdowns
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
    
@app.get("/available_dates/{appliance_name}", response_class=JSONResponse) # Retrieves list of dates with data for a given appliance name from the database for the web and mobile app dropdowns
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

if __name__ == "__main__":      # Starts the app
    uvicorn.run(app, host="0.0.0.0", port=6543)