from fastapi import FastAPI, Request, Form     #imports
from fastapi.responses import Response
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles   # Used for serving static files
import uvicorn
import os
import json
import mysql.connector as mysql
from dotenv import load_dotenv

load_dotenv()

# Read Database connection variables
db_host = "localhost"
db_user = "root"
db_pass = os.environ['MYSQL_ROOT_PASSWORD']
db_name = "ShutEyeDataServer"                           

app = FastAPI()     #initialize the web app
app.mount("/static", StaticFiles(directory="static"), name="static")    #mounts the public folder with static files to the app

@app.get("/", response_class=HTMLResponse)       #returns the index HTML page for the default URL path
def get_html() -> HTMLResponse:                 
  with open('index.html') as html:             
    return HTMLResponse(content=html.read())
  
@app.get("/dashboard", response_class=HTMLResponse)       #returns the dashboard HTML page when dashboard is specified for the URL path
def get_dashboard() -> HTMLResponse:
  with open('dashboard.html') as html:          
    return HTMLResponse(content=html.read())

@app.get("/load_dashboard", response_class=JSONResponse)   #used to load the data from the database into a JSON file -> used to display the idea data on the dashboard
def load_dashboard() -> JSONResponse:
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  cursor.execute("select * from Ideas;")
  records = cursor.fetchall()
  db.close()
  response = {}
  for index, row in enumerate(records):    #iterates through the database data to construct the dict
    response[index] = {
      "id": row[0],
      "first_name": row[1],
      "last_name": row[2],
      "summary": row[3],
      "jtbd": row[4],
      "competitors": row[5],
      "price": float(row[6]),
      "cost": float(row[7])
    }
  return JSONResponse(response)                  
  
@app.post("/idea/{idea_id}")       #modifies an idea in the database
def modify_idea(idea_id: str, parameters: dict):
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  field = parameters['field']
  set_val = parameters['set_val']
  if field == 'first_name':                                   #update the value of the selected field
      query = "update Ideas set first_name=%s WHERE id=%s;"
  elif field == 'last_name':
      query = "update Ideas set last_name=%s WHERE id=%s;"
  elif field == 'summary':
      query = "update Ideas set summary=%s WHERE id=%s;"
  elif field == 'jtbd':
      query = "update Ideas set jtbd=%s WHERE id=%s;"
  elif field == 'competitors':
      query = "update Ideas set competitors=%s WHERE id=%s;"
  elif field == 'price':
      query = "update Ideas set price=%s WHERE id=%s;"
  elif field == 'cost':
      query = "update Ideas set cost=%s WHERE id=%s;"
  else:
     return
  value = (set_val, idea_id)
  try:
    cursor.execute(query, value)
  except RuntimeError as err:
    print("runtime error: {0}".format(err))
  db.commit()
  db.close()

@app.delete("/deleteidea/{idea_id}") #deletes an idea from the database
def delete_idea(idea_id: str):
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  query = "delete from Ideas where id = %s;"
  value = (idea_id,)
  try:
    cursor.execute(query, value)
  except RuntimeError as err:
    print("runtime error: {0}".format(err))
  db.commit()
  db.close()

@app.put("/addidea")       #adds an idea in the database
def add_idea(idea_data: dict):
  db = mysql.connect(host=db_host, database=db_name, user=db_user, passwd=db_pass)
  cursor = db.cursor()
  query = "insert into Ideas(first_name, last_name, summary, jtbd, competitors, price, cost) values (%s, %s, %s, %s, %s, %s, %s)"
  value = (idea_data['first_name'], idea_data['last_name'], idea_data['summary'], idea_data['jtbd'], idea_data['competitors'], idea_data['price'], idea_data['cost'])  #uses dict passed in to PUT to set values of fields
  try:
    cursor.execute(query, value)
  except RuntimeError as err:
    print("runtime error: {0}".format(err))
  db.commit()
  db.close()

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=6543)     #starts the app
