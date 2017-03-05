
// oneviewHTTP.c

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */


#include "oneview.h"
#include "oneviewHTTP.h"
#include "oneviewInfraKitConsole.h"

// CURL Header
#include <curl/curl.h>

#include <stdlib.h>
#include <string.h>


char *httpsAuth;    // String set as User:Pass
const char *httpData;     // Data to send to web service
long portNumber;     // Set the port to be used by curl
int httpMethod;     // Method used to send Data to WebServer

struct curl_slist *headers = NULL;

#define BUFFER_SIZE  (1024 * 1024)  /* 1024 KB */

struct write_result
{
    char *data;
    int pos;
};

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
    struct write_result *result = (struct write_result *)stream;
    
    if(result->pos + size * nmemb >= BUFFER_SIZE - 1)
    {
        fprintf(stderr, "error: too small buffer\n");
        return 0;
    }
    
    memcpy(result->data + result->pos, ptr, size * nmemb);
    result->pos += size * nmemb;
    
    return size * nmemb;
}

void setHttpAuth(char* authString)
{
    httpsAuth = authString;
}

void appendHttpHeader(char *header)
{
    // Append Headers to header list
    headers = curl_slist_append(headers, (const char*) header);
}

void createHeader(char *key, const char *data)
{
    char headerData[strlen(key)+strlen(data)];
    //char headerData[56]; //6 for auth: and 48 (33 < 1.20 version) for sessionID
    strcpy(headerData, key);
    strcat(headerData, data);
    appendHttpHeader(headerData);
}


 /*
  * createURL creates URLs to be sent to HPE OneView, these URLs/URIs make up the REST architecture
  */

void createURL(oneviewSession *session, char *uri)
{
    createURLWithQuery(session, NULL, uri);
}

void createURLWithQuery(oneviewSession *session, oneviewQuery *query, char *uri)
{
    if (((session) && session->address) && strlen(session->address) > 0) { // Ensure that our session exists and an address has been entered
        // Ensure that the address memory is clear before writing
        memset(session->debug->usedAddress, 0, 1024);
        if (!query) { // No query, typically for a POST
            snprintf(session->debug->usedAddress, 1024, "https://%s%s", session->address, uri);
        } else { // A query needs constructing typically for a GET
            // build up a bit mask for the query type
            int queryMask = 0;
            if ((query->count > 0) && query->start < query->count)
                queryMask = queryMask + 1;
            if (query->query)
                queryMask = queryMask + 2;
            if (query->filter)
                queryMask = queryMask + 4;
            
            // Need to invoke curl so that we can make use of curl_easy_escape that will automatically escape characters in the URL (e.g. spaces into %20)
            CURL *curl = curl_easy_init();
            
            switch (queryMask) {
                case 1:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s&count=%d", session->address, uri, query->count);
                    break;
                case 2:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?query=%s", session->address, uri, curl_easy_escape(curl, query->query, 0));
                    break;
                case 3:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?count=%d&?query=\"%s\"", session->address, uri, query->count, curl_easy_escape(curl, query->query, 0));
                    break;
                case 4:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?filter=%s", session->address, uri, curl_easy_escape(curl, query->filter, 0));
                    break;
                case 5:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?count=%d&?filter=\"%s\"", session->address, uri, query->count, curl_easy_escape(curl, query->filter, 0));
                    break;
                case 6:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?query=\"%s\"?filter=\"%s\"", session->address, uri, curl_easy_escape(curl, query->query, 0), curl_easy_escape(curl, query->filter, 0));
                    break;
                case 7:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s?count=%d&?query=\"%s\"?filter=\"%s\"", session->address, uri, query->count, curl_easy_escape(curl, query->query, 0), curl_easy_escape(curl, query->filter, 0));
                    break;
                default:
                    snprintf(session->debug->usedAddress, 1024, "https://%s%s", session->address, uri);
                    break;
            }
            // Finished with curl, so free the instance
            curl_easy_cleanup(curl);
            
        }
    }
    
}


void setHttpPort(long httpPort)
{
    portNumber = httpPort;
}

void setHttpData(const char* dataString)
{
    httpData = dataString;
}

void SetHttpMethod(int method)
{
    if (method > 3) {
        printf("\nError: Unknown HTTP Method\n");
        exit(-1);
    }
    httpMethod = method;
}

// httpRequest function
char *httpFunction(char *url)
{
    if (!url) {
        printf("\n No URL specified\n");
        return "";
    }
    
    CURL *curl = NULL;
    CURLcode status;
    char *data = NULL;
    long code;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl)
        goto error;
    
    data = malloc(BUFFER_SIZE);
    if(!data)
        goto error;
    
    struct write_result write_result = {
        .data = data,
        .pos = 0
    };
    
    if (portNumber != 0) {
        curl_easy_setopt(curl, CURLOPT_PORT, portNumber);
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0); /* This is due to self signed Certs */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* HPE OneView needs a Content-Type setting*/
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15); // Give the connection process a 10 second timeout.
    if (httpMethod < 2) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, httpData);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
    
    switch (httpMethod) {
        case DCHTTPPOST:
            break;
        case DCHTTPPUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            break;
        case DCHTTPGET:
            break;
        case DCHTTPDELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        default:
            break;
    }
//    if (httpsAuth) {
//        curl_easy_setopt(curl, CURLOPT_USERPWD, httpsAuth);
//    }
    
    status = curl_easy_perform(curl);
    if(status != 0)
    {
        fprintf(stderr, "[ERROR] unable to request data from %s:\n", url);
        fprintf(stderr, "%s\n", curl_easy_strerror(status));
        goto error;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    
    switch (code) {
        case 200:
            ovPrintDebug(getPluginTime(), "POST returned 200");
            break;
        case 201:
            ovPrintDebug(getPluginTime(), "POST returned 201, and has been submitted");
            break;
        case 202:
            ovPrintDebug(getPluginTime(), "POST returned 202, and has been accepted");
            break;
        case 400:
//                    printf(ANSI_COLOR_RED "[DEBUG]" ANSI_COLOR_RESET " Malformed request something in the JSON is broken");
            ovPrintWarning(getPluginTime(), "POST returned 400, Malformed request as something in the JSON is broken");
            if (getPluginTime() >= LOGDEBUG) {
            if (data)
                printf("%s", data);
            }
            break;
        case 401:
            ovPrintWarning(getPluginTime(), "POST returned 401, POST failure (usually and authentication issue)");
            break;
        case 409:
            ovPrintWarning(getPluginTime(), "POST returned 409, Conflict error, the request \"may\" have succeeded");
            break;
        default:
            break;
    }
    
    if(code > 500)
    {
        ovPrintError(getPluginTime(), "Error 500+");
        goto error;
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    // Set headers to NULL so that they can be reallocated by headers_append()
    headers = NULL;
    //curl_global_cleanup();
    
    /* zero-terminate the result */
    data[write_result.pos] = '\0';
    
    return data;
    
error:
    if(data)
        free(data);
    if(curl)
        curl_easy_cleanup(curl);
    if(headers) {
        curl_slist_free_all(headers);
        headers = NULL;
    }
    curl_global_cleanup();
    return NULL;
}

void PrintHttpAuth()
{
    if (httpsAuth) {
        printf ("%s\n", httpsAuth);
    }
}
