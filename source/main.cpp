/* Sockets Example
 * Copyright (c) 2016-2020 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "wifi_helper.h"
#include "mbed-trace/mbed_trace.h"

#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include <cstdio>

#include <map>
#include <string>

#define SCALE_MULTIPLIER 0.004

#if MBED_CONF_APP_USE_TLS_SOCKET
#include "root_ca_cert.h"

#ifndef DEVICE_TRNG
#error "mbed-os-example-tls-socket requires a device which supports TRNG"
#endif
#endif // MBED_CONF_APP_USE_TLS_SOCKET

Semaphore printf_sem(1);
InterruptIn button(BUTTON1);

class SocketDemo {
    static constexpr size_t MAX_NUMBER_OF_ACCESS_POINTS = 10;
    static constexpr size_t MAX_MESSAGE_RECEIVED_LENGTH = 100;

#if MBED_CONF_APP_USE_TLS_SOCKET
    static constexpr size_t REMOTE_PORT = 443; // tls port
#else
    static constexpr size_t REMOTE_PORT = 65431; // standard HTTP port
#endif // MBED_CONF_APP_USE_TLS_SOCKET

    bool send_error = false;
    bool send_toggle = true;
    // Thread thread_acc;
    // Thread thread_gyro;
    Thread thread_sensor;
    Thread thread_error;
    Thread thread_toggling;

public:
    SocketDemo() : _net(NetworkInterface::get_default_instance())
    {
        
    }

    ~SocketDemo()
    {
        if (_net) {
            _net->disconnect();
        }
    }

    void run()
    {
        if (!_net) {
            printf("Error! No network interface found.\r\n");
            return;
        }

        /* if we're using a wifi interface run a quick scan */
        if (_net->wifiInterface()) {
            /* the scan is not required to connect and only serves to show visible access points */
            wifi_scan();

            /* in this example we use credentials configured at compile time which are used by
             * NetworkInterface::connect() but it's possible to do this at runtime by using the
             * WiFiInterface::connect() which takes these parameters as arguments */
        }

        /* connect will perform the action appropriate to the interface type to connect to the network */

        printf("Connecting to the network...\r\n");

        nsapi_size_or_error_t result = _net->connect();
        if (result != 0) {
            printf("Error! _net->connect() returned: %d\r\n", result);
            return;
        }

        print_network_info();

        /* opening the socket only allocates resources */
        result = _socket.open(_net);
        if (result != 0) {
            printf("Error! _socket.open() returned: %d\r\n", result);
            return;
        }

#if MBED_CONF_APP_USE_TLS_SOCKET
        result = _socket.set_root_ca_cert(root_ca_cert);
        if (result != NSAPI_ERROR_OK) {
            printf("Error: _socket.set_root_ca_cert() returned %d\n", result);
            return;
        }
        _socket.set_hostname(MBED_CONF_APP_HOSTNAME);
#endif // MBED_CONF_APP_USE_TLS_SOCKET

        /* now we have to find where to connect */

        SocketAddress address;

        if (!resolve_hostname(address)) {
            return;
        }

        address.set_port(REMOTE_PORT);

        /* we are connected to the network but since we're using a connection oriented
         * protocol we still need to open a connection on the socket */

        printf("Opening connection to remote port %d\r\n", REMOTE_PORT);

        result = _socket.connect(address);
        if (result != 0) {
            printf("Error! _socket.connect() returned: %d\r\n", result);
            return;
        }

        /* exchange an HTTP request and response */

        // if (!send_http_request()) {
        //     return;
        // }

        // if (!receive_http_response()) {
        //     return;
        // }

        // if(!send_acc_sensor()){
        //     _socket.close();
        //     printf("Sending Error\r\n");
        //     return;
        // }
        // if(!send_gyro_sensor()){
        //     _socket.close();
        //     printf("Sending Error\r\n");
        //     return;
        // }
        
        button.fall(callback(this, &SocketDemo::button_pressed));

        // thread_acc.start(callback(this, &SocketDemo::send_acc_sensor));
        // thread_gyro.start(callback(this, &SocketDemo::send_gyro_sensor));
        thread_sensor.start(callback(this, &SocketDemo::send_sensor));
        thread_error.start(callback(this, &SocketDemo::check_error));
        thread_toggling.start(callback(this, &SocketDemo::check_toggling));

        // printf("Demo concluded successfully \r\n");        
    }

private:
    bool resolve_hostname(SocketAddress &address)
    {
        const char hostname[] = MBED_CONF_APP_HOSTNAME;

        /* get the host address */
        printf("\nResolve hostname %s\r\n", hostname);
        nsapi_size_or_error_t result = _net->gethostbyname(hostname, &address);
        if (result != 0) {
            printf("Error! gethostbyname(%s) returned: %d\r\n", hostname, result);
            return false;
        }

        printf("%s address is %s\r\n", hostname, (address.get_ip_address() ? address.get_ip_address() : "None") );

        return true;
    }

    bool send_http_request()
    {
        /* loop until whole request sent */
        const char buffer[] = "GET / HTTP/1.1\r\n"
                              "Host: ifconfig.io\r\n"
                              "Connection: close\r\n"
                              "\r\n";

        nsapi_size_t bytes_to_send = strlen(buffer);
        nsapi_size_or_error_t bytes_sent = 0;

        printf("\r\nSending message: \r\n%s", buffer);

        while (bytes_to_send) {
            bytes_sent = _socket.send(buffer + bytes_sent, bytes_to_send);
            if (bytes_sent < 0) {
                printf("Error! _socket.send() returned: %d\r\n", bytes_sent);
                return false;
            } else {
                printf("sent %d bytes\r\n", bytes_sent);
            }

            bytes_to_send -= bytes_sent;
        }

        printf("Complete message sent\r\n");

        return true;
    }

    bool receive_http_response()
    {
        char buffer[MAX_MESSAGE_RECEIVED_LENGTH];
        int remaining_bytes = MAX_MESSAGE_RECEIVED_LENGTH;
        int received_bytes = 0;

        /* loop until there is nothing received or we've ran out of buffer space */
        nsapi_size_or_error_t result = remaining_bytes;
        while (result > 0 && remaining_bytes > 0) {
            result = _socket.recv(buffer + received_bytes, remaining_bytes);
            if (result < 0) {
                printf("Error! _socket.recv() returned: %d\r\n", result);
                return false;
            }

            received_bytes += result;
            remaining_bytes -= result;
        }

        /* the message is likely larger but we only want the HTTP response code */

        printf("received %d bytes:\r\n%.*s\r\n\r\n", received_bytes, strstr(buffer, "\n") - buffer, buffer);

        return true;
    }

    void send_sensor(void){
        BSP_ACCELERO_Init();
        BSP_GYRO_Init();

        uint8_t sample_num = 0;
        int16_t pDataXYZ[3] = {0};
        float pGyroDataXYZ[3] = {0};

        std::map<std::string, float> env_data;
        nsapi_size_or_error_t bytes_sent = 0;

        _socket.set_blocking(1);

        while (true){
            if(!send_error&&send_toggle){
                char acc_json[100];

                printf_sem.acquire();
                if(sample_num==255){
                    sample_num = 0;
                }
                else{
                    ++sample_num;
                }
                BSP_ACCELERO_AccGetXYZ(pDataXYZ);
                env_data["ACCELERO_X"] = pDataXYZ[0];
                env_data["ACCELERO_Y"] = pDataXYZ[1];
                env_data["ACCELERO_Z"] = pDataXYZ[2];
                BSP_GYRO_GetXYZ(pGyroDataXYZ);
                env_data["GYRO_X"] = pGyroDataXYZ[0];
                env_data["GYRO_Y"] = pGyroDataXYZ[1];
                env_data["GYRO_Z"] = pGyroDataXYZ[2];
                // float x = pDataXYZ[0]*SCALE_MULTIPLIER, y = pDataXYZ[1]*SCALE_MULTIPLIER, z = pDataXYZ[2]*SCALE_MULTIPLIER;
                // int len = sprintf(acc_json,"{\"acc_x\":%f,\"acc_y\":%f,\"acc_z\":%f,\"acc_s\":%d}", x, y, z, sample_num);
                // nsapi_error_t response = _socket.send(acc_json,len);
                // nsapi_error_t response = _socket.send(env_data,len);
                nsapi_size_t bytes_to_send  = sprintf(
                    acc_json,
                    "{\"x\":%f, \"y\":%f, \"z\":%f, \"s\":%d}",
                    (float)((int)(env_data["ACCELERO_X"]*10000)) / 10000,
                    (float)((int)(env_data["ACCELERO_Y"]*10000)) / 10000, 
                    (float)((int)(env_data["ACCELERO_Z"]*10000)) / 10000, 
                    sample_num
                );

                while (bytes_to_send) {
                    printf("\r\nbytes to send: %d, bytes_sent: %d\r\n", bytes_to_send, bytes_sent);
                    bytes_sent = _socket.send(acc_json, bytes_to_send);
                    if (bytes_sent < 0) {
                        printf("Error! _socket.send() returned: %d\r\n", bytes_sent);
                    } else {
                        printf("sent %d bytes\r\n", bytes_sent);
                    }

                    bytes_to_send -= bytes_sent;
                }

                // if (0 >= response){
                //     printf("Error sending: %d\n", response);
                //     send_error = true;
                // }
                // else{
                //     printf("\"acc_x\":%.4f,\"acc_y\":%.4f,\"acc_z\":%.4f,\"acc_s\":%d\r\n", x, y, z, sample_num);
                // }
                printf_sem.release();
            }
            ThisThread::sleep_for(1000);
        }

        printf_sem.acquire();
        printf("Send Sensor OK!!\r\n");
        // _socket.close();
        printf_sem.release();
    }

    void send_acc_sensor(void){
        BSP_ACCELERO_Init();

        uint8_t sample_num = 0;
        int16_t pDataXYZ[3] = {0};

        char acc_json[64];

        _socket.set_blocking(1);

        while (true){
            if(!send_error&&send_toggle){
                printf_sem.acquire();
                if(sample_num==255){
                    sample_num = 0;
                }
                else{
                    ++sample_num;
                }
                BSP_ACCELERO_AccGetXYZ(pDataXYZ);
                float x = pDataXYZ[0]*SCALE_MULTIPLIER, y = pDataXYZ[1]*SCALE_MULTIPLIER, z = pDataXYZ[2]*SCALE_MULTIPLIER;
                int len = sprintf(acc_json,"{\"acc_x\":%f,\"acc_y\":%f,\"acc_z\":%f,\"acc_s\":%d}", x, y, z, sample_num);
                nsapi_error_t response = _socket.send(acc_json,len);
                if (0 >= response){
                    printf("Error sending: %d\n", response);
                    send_error = true;
                }
                else{
                    printf("\"acc_x\":%.4f,\"acc_y\":%.4f,\"acc_z\":%.4f,\"acc_s\":%d\r\n", x, y, z, sample_num);
                }
                printf_sem.release();
            }
            ThisThread::sleep_for(1000);
        }

        printf_sem.acquire();
        printf("Send Accelerate Sensor OK!!\r\n");
        // _socket.close();
        printf_sem.release();
    }

    void send_gyro_sensor(void){
        BSP_GYRO_Init();

        uint8_t sample_num = 0;
        float pGyroDataXYZ[3] = {0};

        char gyro_json[64];

        _socket.set_blocking(1);

        while(true){
            if(!send_error&&send_toggle){
               printf_sem.acquire();
                if(sample_num==255){
                    sample_num = 0;
                }
                else{
                    ++sample_num;
                }
                BSP_GYRO_GetXYZ(pGyroDataXYZ);
                float x = pGyroDataXYZ[0], y = pGyroDataXYZ[0], z = pGyroDataXYZ[0];
                int len = sprintf(gyro_json,"{\"gyro_x\":%.4f,\"gyro_y\":%.4f,\"gyro_z\":%.4f,\"gyro_s\":%d}", x, y, z, sample_num);
                nsapi_error_t response = _socket.send(gyro_json,len);
                if (0 >= response){
                    printf("Error sending: %d\n", response);
                    send_error = true;
                }
                else{
                    printf("\"gyro_x\":%.4f,\"gyro_y\":%.4f,\"gyro_z\":%.4f,\"gyro_s\":%d\r\n", x, y, z, sample_num);
                }
                printf_sem.release();
            }
            ThisThread::sleep_for(1000);
        }

        printf_sem.acquire();
        printf("Send Gyro Sensor OK!!\r\n");
        // _socket.close();
        printf_sem.release();
    }

    void check_error(void){
        while (true) {
            if(send_error){
                _socket.close();
                printf_sem.acquire();
                printf("Socket Error!!\r\n");
                printf_sem.release();
            }
            ThisThread::sleep_for(1000);
        }
    }

    void check_toggling(void){
        while (true) {
            if(!send_toggle){
                printf_sem.acquire();
                printf("Toggling!!\r\n");
                printf_sem.release();
            }
            ThisThread::sleep_for(1000);
        }
    }

    void button_pressed(void){
        send_toggle = !send_toggle;
    }

    void wifi_scan()
    {
        WiFiInterface *wifi = _net->wifiInterface();

        WiFiAccessPoint ap[MAX_NUMBER_OF_ACCESS_POINTS];

        /* scan call returns number of access points found */
        int result = wifi->scan(ap, MAX_NUMBER_OF_ACCESS_POINTS);

        if (result <= 0) {
            printf("WiFiInterface::scan() failed with return value: %d\r\n", result);
            return;
        }

        printf("%d networks available:\r\n", result);

        for (int i = 0; i < result; i++) {
            printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n",
                   ap[i].get_ssid(), get_security_string(ap[i].get_security()),
                   ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
                   ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
                   ap[i].get_rssi(), ap[i].get_channel());
        }
        printf("\r\n");
    }

    void print_network_info()
    {
        /* print the network info */
        SocketAddress a;
        _net->get_ip_address(&a);
        printf("IP address: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_netmask(&a);
        printf("Netmask: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
        _net->get_gateway(&a);
        printf("Gateway: %s\r\n", a.get_ip_address() ? a.get_ip_address() : "None");
    }

private:
    NetworkInterface *_net;

#if MBED_CONF_APP_USE_TLS_SOCKET
    TLSSocket _socket;
#else
    TCPSocket _socket;
#endif // MBED_CONF_APP_USE_TLS_SOCKET
};

int main() {
    printf("\r\nStarting socket demo\r\n\r\n");

#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    SocketDemo *example = new SocketDemo();
    MBED_ASSERT(example);
    example->run();
}
