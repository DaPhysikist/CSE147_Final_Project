# Add the necessary imports
import mysql.connector as mysql
import os
import datetime
from dotenv import load_dotenv

load_dotenv()   #Load environment file

# Read Database connection variables
db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']

# Connect to the db and create a cursor object
db = mysql.connect(user=db_user, password=db_pass, host=db_host)
cursor = db.cursor()

cursor.execute("CREATE DATABASE if not exists ShutEyeDataServer")  #create database
cursor.execute("USE ShutEyeDataServer")

cursor.execute("drop table if exists ShutEyeDeviceEnergyDataHistorical;")   #creates Ideas table with fields, drops table if it exists already -> used to repopulate table with data when file run again
try:
   cursor.execute("""
   CREATE TABLE ShutEyeData (
      appliance_name     VARCHAR(50) NOT NULL PRIMARY KEY,
      local_time         DATETIME NOT NULL,
      today_runtime      INTEGER NOT NULL,
      month_runtime      INTEGER NOT NULL,
      today_energy       INTEGER NOT NULL,
      month_energy       INTEGER NOT NULL,
   );
""")
except RuntimeError as err:
   print("runtime error: {0}".format(err))

cursor.execute("drop table if exists ShutEyeCurrentDeviceaData;")   #creates Ideas table with fields, drops table if it exists already -> used to repopulate table with data when file run again
try:
   cursor.execute("""
   CREATE TABLE ShutEyeData (
       id                 INTEGER AUTO_INCREMENT PRIMARY KEY,
       appliance_name     VARCHAR(50) NOT NULL,
       current_power      INTEGER NOT NULL,
      distance_ultrasonic INTEGER NOT NULL,
      distance_bluetooth  INTEGER NOT NULL,
      distance_ultrawideband INTEGER NOT NULL,
      user_presence_detected BOOLEAN NOT NULL
   );
""")
except RuntimeError as err:
   print("runtime error: {0}".format(err))

db.commit()  #finishes database initialization
db.close()