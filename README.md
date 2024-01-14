# matter-demo
using esp32 device and google cloud, build a simple Matter demo to show data transfer from device to web dashboard through Matter.

# It has multiple platform components
* esp-idf
* google cloud

# esp-idf
It uses esp32s3 board in the first stage. Because esp32s3 has no thread or zigbee wireless, need to change the platform to another that has proper wireless for Matter, later.

# google cloud
It uses google cloud AppEngine Python, Firestore platform to simplify the web dashboard application.
