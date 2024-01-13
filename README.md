# matter-demo
using esp32 device and google cloud, build a matter demo to show data flow from device to web dashboard through matter.

# It has multiple platform components
* esp-idf
* google cloud

# esp-idf
It uses esp32s3 board temporary in the first stage. Because esp32s3 has no thread or zigbee wireless component, need to change the platform for Matter.

# google cloud
It uses google cloud AppEngine Python, Firestore platform to simplify the project.
