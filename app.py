import os
import sys
import json

import cv2
from flask import Flask, render_template, Response, request, url_for, flash, redirect, jsonify
from werkzeug.exceptions import abort

import redis
redis_conn = redis.Redis()
try:
    IS_REDIS_CONNECTED = redis_conn.ping()
except:
    IS_REDIS_CONNECTED = False

app = Flask(__name__)
app.config["SECRET_KEY"] = "admin"

capture = cv2.VideoCapture("./templates/test-0.mp4")


@app.route("/")
def index():
    return render_template('index.html')

@app.route("/rover_control", methods=["POST"])
def rover_control():
    jdata = request.json
    thrust = jdata.get("thrust")
    steering = jdata.get("steering")
    print ("Thrust:", thrust, "Steering:", steering)

    if IS_REDIS_CONNECTED:
        redis_conn.mset({"thrust": thrust, "steering": steering})
    return ('/')

@app.route("/rover_control_start_stop", methods=["POST"])
def rover_control_start_stop():
    jdata = request.json
    isRoverStopped = jdata.get("isRoverStopped")
    print("isRoverStopped?", isRoverStopped)
    if IS_REDIS_CONNECTED:
        redis_conn.mset({"isRoverStopped": isRoverStopped})
    return ('/')

@app.route("/rover_control_light", methods=["POST"])
def rover_control_light():
    jdata = request.json
    isLightOn = jdata.get("isLightOn")
    print("isLightOn?", isLightOn)
    if IS_REDIS_CONNECTED:
        redis_conn.mset({"isLightOn": isLightOn})
    return ('/')

@app.route("/rover_control_mode_switch", methods=["POST"])
def rover_control_mode_switch():
    jdata = request.json
    isManuelModeActive = jdata.get("isManuelModeActive")
    print("isManuelModeActive?", isManuelModeActive)
    if IS_REDIS_CONNECTED:
        redis_conn.mset({"isManuelModeActive": isManuelModeActive})
    return ('/')

@app.route("/settings")
def settings():
    return render_template('settings.html')

@app.route("/about")
def about():
    return render_template('about.html')

def gen_frames():
    global capture
    while True:
        success, frame = capture.read()
        if not success:
            capture = cv2.VideoCapture("./templates/test-0.mp4")
            break
        else:
            ret, buffer = cv2.imencode('.jpg', frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')  # concat frame one by one and show result

@app.route('/video_feed')
def video_feed():
    cv2.waitKey(100)
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')


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