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

app = Flask(__name__)

# Initialize Firestore DB
db = firestore.Client()

@app.route('/')
def home():
    temp_ref = db.collection('temperatures').document('latest')
    temp_doc = temp_ref.get()
    if temp_doc.exists:
        data = temp_doc.to_dict()
        temperature = data.get('value')

        # Extract timestamp and convert it to a datetime object
        timestamp = data.get('time')
        if timestamp:
            # Assuming the timestamp is in UTC and you want to convert it to local time
            local_timezone = pytz.timezone('Asia/Seoul')  # Replace 'Your_Timezone' with your local timezone
            updated_time = timestamp.strftime('%Y-%m-%d %H:%M:%S')
        else:
            updated_time = 'No timestamp'
    else:
        temperature = 'No data'
        updated_time = 'No data'

    return render_template('index.html', temperature=temperature, updated_time=updated_time)
"""
use this curl command to test
for test:
curl -X POST "http://localhost:8080/temperature" -H "Content-Type: application/json" -d "{\"temperature\": 25.5}"
for production:
curl -X POST "https://model-craft-409812.du.r.appspot.com/temperature" -H "Content-Type: application/json" -d "{\"temperature\": 25.5}"

use this HTTP command:
POST /temperature HTTP/1.1
Host: model-craft-409812.du.r.appspot.com
Content-Type: application/json
Content-Length: 24

{"temperature": 25.5}
"""
@app.route('/temperature', methods=['POST'])
def post_temperature():
    try:
        # Get the temperature value from the request
        request_data = request.get_json()
        temperature = request_data['temperature']

        # get the latest document in the Firestore database
        temp_ref = db.collection('temperatures').document('latest')
        temp_ref.set({
            'value': temperature,
            'time': firestore.SERVER_TIMESTAMP
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
