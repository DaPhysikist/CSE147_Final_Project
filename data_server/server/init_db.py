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

cursor.execute("drop table if exists ShutEyeData;")   #creates Ideas table with fields, drops table if it exists already -> used to repopulate table with data when file run again
try:
   cursor.execute("""
   CREATE TABLE ShutEyeData(
       id          integer  AUTO_INCREMENT PRIMARY KEY,
       first_name        VARCHAR(50) NOT NULL,
       last_name        VARCHAR(50) NOT NULL,
       summary    TEXT NOT NULL,    
       jtbd       TEXT NOT NULL,
       competitors       VARCHAR(1000) NOT NULL,
       price       DECIMAL(12,2) NOT NULL,
       cost        DECIMAL(12,2) NOT NULL
   );
 """)
except RuntimeError as err:
   print("runtime error: {0}".format(err))

db.commit()  #finishes database initialization
db.close()