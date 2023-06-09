import os
import sys
import json

from flask import Flask, render_template, request, url_for, flash, redirect, jsonify
from werkzeug.exceptions import abort

import redis
redis_conn = redis.Redis()
try:
    IS_REDIS_CONNECTED = redis_conn.ping()
except:
    IS_REDIS_CONNECTED = False

app = Flask(__name__)
app.config["SECRET_KEY"] = "admin"

@app.route("/")
def index():
    return render_template('index.html')

@app.route("/rover_control/<string:thrust>_<string:steering>", methods=["POST"])
def rover_control(thrust, steering):
    thrust = json.loads(thrust)
    steering = json.loads(steering)
    print (thrust, steering)

    if IS_REDIS_CONNECTED:
        redis_conn.mset({"thrust": thrust, "steering": steering})
    return ('/')

@app.route("/settings")
def settings():
    return render_template('settings.html')

@app.route("/about")
def about():
    return render_template('about.html')

if __name__ == "__main__":
    # TODO orientation info of mobile device is only obtained with https path so ssl certificate is needed.
    # You need to generate ssl certificate prior to run the project. Apply following steps:
    # 1-Run following commands in the terminal:
    #   $ openssl genrsa -des3 -out server.key 1024
    #   $ openssl req -new -key server.key -out server.csr
    #   $ cp server.key server.key.org
    #   $ openssl rsa -in server.key.org -out server.key
    #   $ openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
    # 2- Include .crt and .key files into the project folder:
    
    context = ('server.crt', 'server.key')  # certificate and key files

    if os.path.exists(context[0]) and os.path.exists(context[1]):
        app.run(host="0.0.0.0", debug=True, ssl_context=context)
    else:
        app.run(host="0.0.0.0", debug=True)