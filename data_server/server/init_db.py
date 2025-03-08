import mysql.connector as mysql
import os
from dotenv import load_dotenv

load_dotenv()

db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']

db = mysql.connect(user=db_user, password=db_pass, host=db_host)
cursor = db.cursor()

cursor.execute("CREATE DATABASE IF NOT EXISTS ShutEyeDataServer")
cursor.execute("USE ShutEyeDataServer")

cursor.execute("DROP TABLE IF EXISTS ShutEyeDeviceEnergyDataHistorical")
try:
    cursor.execute("""
    CREATE TABLE ShutEyeDeviceEnergyDataHistorical (
        appliance_name     VARCHAR(50) NOT NULL,
        local_time         DATETIME NOT NULL,
        today_runtime      INTEGER NOT NULL,
        month_runtime      INTEGER NOT NULL,
        today_energy       INTEGER NOT NULL,
        month_energy       INTEGER NOT NULL,
        PRIMARY KEY (appliance_name, local_time)
    );
    """)
except RuntimeError as err:
    print("Runtime error: {0}".format(err))

cursor.execute("DROP TABLE IF EXISTS ShutEyeDeviceEnergyDataPeriodicMeasurement")
try:
    cursor.execute("""
    CREATE TABLE ShutEyeDeviceEnergyDataPeriodicMeasurement (
        appliance_name           VARCHAR(50) NOT NULL,
        local_time               DATETIME NOT NULL,
        current_power            INTEGER NOT NULL,
        distance_ultrasonic      INTEGER NOT NULL,
        distance_bluetooth       INTEGER NOT NULL,
        distance_ultrawideband   INTEGER NOT NULL,
        user_presence_detected   BOOLEAN NOT NULL,
        PRIMARY KEY (appliance_name, local_time)
    );
    """)
except RuntimeError as err:
    print("Runtime error: {0}".format(err))

db.commit()
db.close()