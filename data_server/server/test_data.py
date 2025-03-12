import mysql.connector as mysql
import os
import random
from datetime import datetime, timedelta
from dotenv import load_dotenv

# Load environment variables (if needed)
load_dotenv()

# MySQL Database Connection
db_host = "localhost"
db_user = "root"
db_pass = os.getenv("MYSQL_ROOT_PASSWORD")  # Load from environment
db_name = "ShutEyeDataServer"

# Connect to MySQL
try:
    db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
    cursor = db.cursor()
    print("Connected to MySQL Database")
except mysql.Error as err:
    print(f"Database connection error: {err}")
    exit()

# Start and end times for two days
start_time = datetime(2025, 3, 1, 0, 0, 0)
end_time = datetime(2025, 3, 2, 23, 59, 50)
interval = timedelta(seconds=10)  # 10-second intervals

# SQL Insert Query
query = """
INSERT INTO ShutEyeDeviceEnergyDataPeriodicMeasurement 
(appliance_name, local_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected) 
VALUES (%s, %s, %s, %s, %s, %s, %s);
"""

# Generate Data
data = []
current_time = start_time

while current_time <= end_time:
    for appliance_name in ["microwave", "blender"]:  # Insert both appliances every timestamp
        current_power = random.randint(140, 180)  # Simulate power usage in watts
        distance_ultrasonic = random.randint(18, 26)  # Simulated sensor data (cm)
        distance_bluetooth = random.randint(26, 35)  # Simulated sensor data (cm)
        distance_ultrawideband = random.randint(44, 60)  # Simulated sensor data (cm)

        # Simulate user presence detection realistically:
        if appliance_name == "blender":
            # Higher chance of user presence during mornings or early afternoons for a blender
            user_presence_detected = random.random() < (0.7 if 6 <= current_time.hour < 14 else 0.2)
        else:
            # Default pattern for the microwave (e.g., more likely in the evening)
            user_presence_detected = random.random() < (0.8 if 16 <= current_time.hour < 22 else 0.3)

        # Add the data for the current time
        data.append((appliance_name, current_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected))

    current_time += interval  # Increment by 10 seconds

# Insert Data in Batches
try:
    cursor.executemany(query, data)
    db.commit()
    print(f"Inserted {len(data)} records into ShutEyeDeviceEnergyDataPeriodicMeasurement.")
except mysql.Error as err:
    print(f"Error inserting data: {err}")
    db.rollback()

# Close Connection
cursor.close()
db.close()
print("MySQL connection closed.")
