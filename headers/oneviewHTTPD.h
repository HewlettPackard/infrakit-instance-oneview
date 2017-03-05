
 // oneviewHTTPD.h
 
/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */

#include <stddef.h>

typedef struct {
    char *socketPath;       // Path to UNIX Socket to pind to
    int port;               // Port to bind server to
} serverConfiguration;

typedef struct {
    // RFC requestLine
    char *method;
    char *URI;
    char *HTTPVersion;
    
    // Helper lines
    char *requestLine;      // Status Line (GET/POST) etc.
    char *headers;          // Headers
    char *messageBody;      // The remaining part (messageBody) of the httpResponse
} httpRequest;

typedef struct {
    // Helper lines
    int responseCode;       // Response to a request
    size_t messageLength;   // Size of the reponse message for Content Length response
    char *messageBody;      // The response message
} httpResponse;


#define BUFFER_SIZE 1024*1024
//#define MAX_FILE_SIZE 5*1024
//#define TRUE 1
//#define FALSE 0
//#define EXIT_SUCCESS 0
//#define EXIT_FAILURE 1
//
//#define OV_HTTPD_GET 1
//#define OV_HTTPD_HEAD 2
//#define OV_HTTPD_POST 3


void startHTTPDServer();
httpRequest *processHttpRequest(char *rawData);
int setSocketPath(char *path);
int setHTTPResponse(char *messageBody, int responseCode);


#ifndef HTTPDCALLBACK_H
#define HTTPDCALLBACK_H
void SetPostFunction( char *(*postCallbackFunction)(httpRequest *));
#endif
