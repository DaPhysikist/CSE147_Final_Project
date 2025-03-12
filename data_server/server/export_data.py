import mysql.connector as mysql
import csv
import os
from dotenv import load_dotenv

load_dotenv()

db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']

db = mysql.connect(user=db_user, password=db_pass, host=db_host)
cursor = db.cursor()
cursor.execute("USE ShutEyeDataServer")
cursor.execute("SELECT * FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = 'lamp'")  
rows = cursor.fetchall()

# Get column names
column_names = [i[0] for i in cursor.description]

# Define CSV file path
csv_file_path = "lamp_data.csv"  # Adjust the path as needed

# Write data to CSV file
with open(csv_file_path, mode="w", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(column_names)  # Write column headers
    writer.writerows(rows)  # Write row data

print(f"Database contents exported to {csv_file_path}")

db.commit()
db.close()
