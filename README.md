#Closing this project
This project is being closed to start a new one.
The example demonstrates how a simple ESP32 device can send a POST message to a Google Cloud app.
The plan was to apply Matter network in the middle of the message loop, but Matter network was not started and the project was stopped
Thank you and see you later.

# matter-demo
using esp32 device and google cloud, build a simple Matter demo to show data transfer from device to web dashboard through Matter.

# It has multiple platform components
* esp-idf
* google cloud

# esp-idf
It uses esp32s3 board in the first stage. Because esp32s3 has no thread or zigbee wireless, need to change the platform to another that has proper wireless for Matter, later.

# google cloud
Leveraging the hello world example that is provide by Google. A web dashboard application that shows simple sensor value (temperature) was made. It uses google cloud AppEngine Python, Firestore platform to simplify the application.
