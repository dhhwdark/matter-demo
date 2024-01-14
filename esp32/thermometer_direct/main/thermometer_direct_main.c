/*
 * HTTPS GET Example using plain Mbed TLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in Mbed TLS.
 *
 * SPDX-FileCopyrightText: The Mbed TLS Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileContributor: 2015-2022 Espressif Systems (Shanghai) CO LTD
 */

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "sdkconfig.h"
#include "esp_crt_bundle.h"
#include "driver/temperature_sensor.h"

#include "time_sync.h"

static const char *TAG = "thermometer_direct";

/* hard coded server information */
const char *HOST_URL = CONFIG_TEMPERATURE_DB_SERVER_URL;
const char *PROTOCOL = "https://";

/* Timer interval once every day (24 Hours) */
#define TIME_PERIOD (86400000000ULL)

/* refer the format
POST /temperature HTTP/1.1\r\n
Host: [HOST_URL]\r\n
User-Agent: esp-idf/1.0 esp32\r\n
Content-Type: application/json\r\n
Content-Length: [LENGTH_OF_BODY]\r\n
\r\n
[BODY]
*/
static const char * TEMPERATURE_REQUEST = "POST /temperature HTTP/1.1\r\n"
                             "Host: %s\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "Content-Type: application/json\r\n"
                             "Content-Length: %d\r\n\r\n%s";
static const char *TEMPERATURE_REQUEST_JSON = "{\"temperature\": %4.2f}";

char *https_build_temperature_post(char *buf, int buf_size, float temperature){
    char json_body [48];
    snprintf(json_body, sizeof(json_body), TEMPERATURE_REQUEST_JSON, temperature);
    snprintf(buf, buf_size, TEMPERATURE_REQUEST, HOST_URL, strlen(json_body), json_body);
    return buf;
}

static esp_tls_client_session_t *tls_client_session = NULL;
temperature_sensor_handle_t temp_sensor = NULL;

void temperature_init (){
    ESP_LOGI(TAG, "Install temperature sensor, expected temp ranger range: 10~50 ℃");
    
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    ESP_LOGI(TAG, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
}

float temperature_get (){
    ESP_LOGI(TAG, "Read temperature");
    float tsens_value;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &tsens_value));
    ESP_LOGI(TAG, "Temperature value %.02f ℃", tsens_value);
    return tsens_value;
}

void https_request_init () {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    if (1 || esp_reset_reason() == ESP_RST_POWERON) {
        ESP_LOGI(TAG, "Updating time from NVS");
        ESP_ERROR_CHECK(update_time_from_nvs());
    }

    const esp_timer_create_args_t nvs_update_timer_args = {
        .callback = (void *)&fetch_and_store_time_in_nvs,
    };

    esp_timer_handle_t nvs_update_timer;
    ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, TIME_PERIOD));
}

static bool https_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST) {
    char buf[512];
    int ret, len;
    bool success_flag = false;
    printf("[don]requesting...\nserver: %s\nrequest: \n%s\n", WEB_SERVER_URL, REQUEST);
    esp_tls_t *tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
        return false;
    }
    if (esp_tls_conn_http_new_sync(WEB_SERVER_URL, &cfg, tls) == 1) {
        ESP_LOGI(TAG, "Connection established...");
    } else {
        ESP_LOGE(TAG, "Connection failed...");
        int esp_tls_code = 0, esp_tls_flags = 0;
        esp_tls_error_handle_t tls_e = NULL;
        esp_tls_get_error_handle(tls, &tls_e);
        /* Try to get TLS stack level error and certificate failure flags, if any */
        ret = esp_tls_get_and_clear_last_error(tls_e, &esp_tls_code, &esp_tls_flags);
        if (ret == ESP_OK) {
            ESP_LOGE(TAG, "TLS error = -0x%x, TLS flags = -0x%x", esp_tls_code, esp_tls_flags);
        }
        goto cleanup;
    }
    /* The TLS session is successfully established, now saving the session ctx for reuse */
    if (tls_client_session == NULL) {
        tls_client_session = esp_tls_get_client_session(tls);
    }
    success_flag = true;
    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 strlen(REQUEST) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto cleanup;
        }
    } while (written_bytes < strlen(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");
    do {
        len = sizeof(buf) - 1;
        memset(buf, 0x00, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);
        ESP_LOGI(TAG, "esp_tls_conn_read ret: %d", ret);
        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        } else if (ret < 0) {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        } else if (ret == 0) {
            ESP_LOGI(TAG, "connection closed");
            break;
        }
        //echo data for debugging
        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for (int i = 0; i < len; i++) {
            putchar(buf[i]);
        }
        putchar('\n'); // JSON output doesn't have a newline at end
        break; // one time connection
    } while (1);
cleanup:
    esp_tls_conn_destroy(tls);
    return success_flag;
}

static void https_report_temperature (float temperature){
    char request_buf [512];
    char web_url [128];
    strcpy(web_url, PROTOCOL);
    strcat(web_url, HOST_URL);
    ESP_LOGI(TAG, "reporting temperature: %4.2f to the WEB: %s", temperature, web_url);
    https_build_temperature_post(request_buf, sizeof(request_buf), temperature);
    bool ret = false;
    if (tls_client_session) {
        esp_tls_cfg_t cfg = {
            .client_session = tls_client_session,
        };
        ESP_LOGI(TAG, "using client session");
        ret = https_request(cfg, web_url, request_buf);
    }
    if (ret == false) {// then try once again
        esp_tls_cfg_t cfg = {
            .crt_bundle_attach = esp_crt_bundle_attach,
        };
        ESP_LOGI(TAG, "using bundle crt");
        https_request(cfg, web_url, request_buf);
    }
}

static void https_request_task(void *pvparameters){
    float prev_temperature = 0.0;
    while(1) { // eternal loop to send temperature data
        float temperature = temperature_get();
        float diff = temperature - prev_temperature;
        if (diff < -0.1 || 0.1 < diff) {
            prev_temperature = temperature;
            https_report_temperature(temperature);
        }
        vTaskDelay(10*1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
    if (tls_client_session) esp_tls_free_client_session(tls_client_session);
    tls_client_session = NULL;
    vTaskDelete(NULL);
}

void app_main(void){
    https_request_init();
    temperature_init();
    xTaskCreate(&https_request_task, "https_get_task", 8192, NULL, 5, NULL);
}
