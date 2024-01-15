# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from flask import Flask, render_template, request, jsonify
from google.cloud import firestore
from datetime import datetime
import pytz
import plotly.graph_objs as go
import plotly.io as pio
import pandas as pd

app = Flask(__name__)

# Initialize Firestore DB
db = firestore.Client()

def convert_to_timezone(time, timezone_str):
    timezone = pytz.timezone(timezone_str)
    if isinstance(time, str):
        time = datetime.fromisoformat(time)
    if time.tzinfo is None or time.tzinfo.utcoffset(time) is None:
        time = pytz.utc.localize(time)
    return time.astimezone(timezone)

@app.route('/')
def home():
    # Fetch data from Firestore
    temperatures_ref = db.collection('temperatures')
    docs = temperatures_ref.stream()

    # Prepare data for plotting
    tdata_list = [{'time': convert_to_timezone(doc.to_dict()['time'], 'Asia/Seoul') , 'temperature': doc.to_dict()['temperature']} for doc in docs]
    tdata_list.sort(key=lambda x: x['time'])

    tdata_list = tdata_list[-100:]
    the_last_temperature = tdata_list[-1].get('temperature')
    the_last_updated_time = convert_to_timezone(tdata_list[-1].get('time'), 'Asia/Seoul').strftime('%Y-%m-%d %H:%M:%S')
    df = pd.DataFrame(tdata_list)

    # Create a Plotly figure
    fig = go.Figure(data=[go.Scatter(x=df['time'], y=df['temperature'], mode='lines+markers')])
    fig.update_layout(
        title='Temperature', 
        xaxis_title='Time', 
        yaxis_title='Temperature (°C)')
    fig.update_layout(
        title={
            'x' : 0.5,
            'xanchor' : 'center'
        })
    # Convert the figure to HTML
    graph_html = pio.to_html(fig, full_html=False)

    return render_template('index.html', graph_html=graph_html, temperature=the_last_temperature, updated_time=the_last_updated_time)

"""
use this curl command to test
for test:
curl -X POST "http://localhost:8080/temperature" -H "Content-Type: application/json" -d "{\"temperature\": 25.5}"
for production:
curl -X POST "https://[your_host]/temperature" -H "Content-Type: application/json" -d "{\"temperature\": 25.5}"

use this HTTP command:
POST /temperature HTTP/1.1
Host: [your_host]
Content-Type: application/json
Content-Length: 24

{"temperature": 25.5}
"""
@app.route('/temperature', methods=['POST'])
def post_temperature():
    try:
        # Get the temperature value from the request
        request_data = request.get_json()
        temperature = request_data.get('temperature')

        # get the latest document in the Firestore database
        # and set with the value
        temp_ref = db.collection('temperatures').document('latest')
        temp_ref.set({
            'temperature': temperature,
            'time': firestore.SERVER_TIMESTAMP
        })
        # and add new value
        doc_ref = db.collection('temperatures').document()
        doc_ref.set({
            'temperature': temperature,
            'time': firestore.SERVER_TIMESTAMP  # Storing the current UTC time
        })
        return jsonify({"success": True, "message": "Temperature recorded successfully."}), 200
    except Exception as e:
        return jsonify({"success": False, "message": str(e)}), 400

if __name__ == "__main__":
    # This is used when running locally only. When deploying to Google App
    # Engine, a webserver process such as Gunicorn will serve the app. You
    # can configure startup instructions by adding `entrypoint` to app.yaml.
    app.run(host="127.0.0.1", port=8080, debug=True)
# [END gae_python3_app]
# [END gae_python38_app]
