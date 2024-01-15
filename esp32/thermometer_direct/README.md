# Thermometer_direct project:
This project was originally copied from ESP-IDE example, https_request. And, Modified to send temperature data to an HOST.
The HOST should be configured by menuconfig.
Also, WiFi SSID and its password should be configured.
The ESP_TLS_CLIENT_SESSION_TICKETS should be set. It is set by default.

# configure HOST
You need to use menuconfig to configure HOST where the device will send a HTTP POST reqeust to update temperature. 

# configure SSID and Password
You need to configure WiFi SSID and its password for your test by referring the https_request example of ESP-IDE
The reference: 
* https://github.com/espressif/esp-idf/tree/master/examples/protocols/https_request
