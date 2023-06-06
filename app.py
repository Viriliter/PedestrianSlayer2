import json
import datetime

from flask import Flask, render_template, request, url_for, flash, redirect, jsonify
from werkzeug.exceptions import abort

app = Flask(__name__)
app.config["SECRET_KEY"] = "admin"

@app.route("/")
def index():
    return render_template('index.html')

@app.route("/settings")
def settings():
    return render_template('settings.html')

@app.route("/about")
def about():
    return render_template('about.html')

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)