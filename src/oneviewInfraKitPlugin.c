
// oneviewInfraKitPlugin.c

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */



#include "oneviewInfraKitConsole.h"
#include "oneviewInfraKitInstance.h"
#include "oneviewInfraKitPlugin.h"
#include "oneviewInfraKitState.h"
#include "oneviewHTTP.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>

#include "jansson.h"

char *argumentSocketPath;
char *argumentLogLevel;

char *socketName = NULL;
char *socketDefault = "instance-oneview";
char socketPath[PATH_MAX];

int setSocketName(char *name)
{
    if (name) {
        socketName = name;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}


/* This callback function will take the request data from the HTTPD server
 * process it and build a response and reponse code, that the web
 * server will then send to the client
 */

char *handlePostData(httpRequest *request)
{
    json_t *requestJSON = NULL;
    json_error_t error;
    
    requestJSON = json_loads(request->messageBody, 0, &error);
    if (requestJSON) {
        const char *methodName = json_string_value(json_object_get(requestJSON, "method"));
        long long id = json_integer_value(json_object_get(requestJSON, "id"));
        json_t *params = json_object_get(requestJSON, "params");

        /* Look through the parameters to determing the group ID, this is
         * needed for multi-tenancy groups. The problem here is that the instance.destroy
         * doesn't have a group tag
         */
        
        const char *groupName = json_string_value(json_object_get(json_object_get(params, "Tags"), "infrakit.group"));
        if (!groupName) {
            json_t *spec = json_object_get(params, "Spec");
            groupName = json_string_value(json_object_get(json_object_get(spec, "Tags"), "infrakit.group"));
        }

        if (groupName) {
            char statePath[PATH_MAX];
            snprintf(statePath, PATH_MAX, "%s/.infrakit/state/%s.json", getenv("HOME"), groupName);
            
            if (!getArgStatePath()) {
                setStatePath(statePath);
            } else {
                setStatePath(getArgStatePath());
            }
        } else {
            ovPrintDebug(getPluginTime(), "No InfraKit Group discovered, this could be a delete function");
        }

        
        const char *debugMessage = json_dumps(requestJSON, JSON_INDENT(3));
        ovPrintDebug(getPluginTime(), "Incoming Request =>");
        ovPrintDebug(getPluginTime(), debugMessage);
        
        if (stringMatch(methodName, "Instance.DescribeInstances")) {

            char *response = ovInfraKitInstanceDescribe(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return NULL;
        }
        if (stringMatch(methodName, "Handshake.Implements")) {
            //{"jsonrpc":"2.0","result":{"APIs":[{"Name":"Instance","Version":"0.1.0"}]},"id":3618748489630577360}

            json_t *reponseJSON = json_pack("{s:s,s:{s:[{s:s,s:s}]},s:I}", "jsonrpc", "2.0", "result", "APIs", "Name", "Instance", "Version", "0.3.0", "id", id);
            char *test = json_dumps(reponseJSON, JSON_ENSURE_ASCII);
            setHTTPResponse(test, 200);
            json_decref(requestJSON);
            return NULL;
        }
        if (stringMatch(methodName, "Instance.Validate")) {
            
            json_t *reponseJSON = json_pack("{s:{s:b},s:s?,s:I}", "result", "OK", JSON_TRUE, "error", NULL, "id", id);
            char *test = json_dumps(reponseJSON, JSON_ENSURE_ASCII);
            setHTTPResponse(test, 200);
            json_decref(requestJSON);
            return NULL;
        }
        
        if (stringMatch(methodName, "Instance.Provision")) {
            
            json_t *spec = json_object_get(params, "Spec");
            char *response = ovInfraKitInstanceProvision(spec, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return NULL;
        }
        if (stringMatch(methodName, "Instance.Destroy")) {
            char *response = ovInfraKitInstanceDestroy(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return NULL;
        }
        if (stringMatch(methodName, "Instance.Meta")) {
            char *response = ovInfraKitInstanceDestroy(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return NULL;
        }
        if (methodName) {
            ovPrintError(getPluginTime(), "Unknown JSON-RPC Method");
        }
        json_decref(requestJSON);
    }
    return NULL;
}

void handleInterrupt(int s){
    printf("Caught signal %d\n",(int)s);
    ovPrintError(getPluginTime(), "InfraKit-instance-oneview is exciting, removing UNIX Socket =>");
    ovPrintError(getPluginTime(), socketPath);
    unlink(socketPath);
}

void setInterruptHandler()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = handleInterrupt;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
}

int ovCreateInfraKitInstance()
{
    setInterruptHandler();
    
    if (setStartTime() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    setConsolOutputLevel(LOGINFO);
    ovPrintInfo(getPluginTime(), "Starting OneView Instance Plugin");
    
    /* These two paths will build out to be the path for the socket and the state
     * we will build them out and ensure that the paths are fully created, including
     * the directory paths.
     */
    
    sprintf(socketPath, "%s/", getenv("HOME"));
    strcat(socketPath, ".infrakit/");
    int response = 0;
    response = mkdir(socketPath, 0755);
    if (response == -1) {
        ovPrintWarning(getPluginTime(), "Directory .InfraKit may already exist");
    }
    
//    
//    char *statePath = getStatePath();
//    if (!statePath) {
//        char buildStatePath[PATH_MAX];
//        sprintf(buildStatePath, "%sstate/",socketPath);
//        response = mkdir(buildStatePath, 0755);
//        if (response == -1) {
//            ovPrintWarning(getPluginTime(), "Directory .InfraKit/state/ may already exist");
//        }
//        strcat(buildStatePath, "oneview.json");
//        statePath = buildStatePath;
//    }

    
    strcat(socketPath, "plugins/");
    response = mkdir(socketPath, 0755);
    if (response == -1) {
        ovPrintWarning(getPluginTime(), "Directory .InfraKit/plugins/ may already exist");
    }
    
    if (socketName) {
        strcat(socketPath, socketName);
    } else {
        strcat(socketPath, socketDefault);
    }
    
    ovPrintInfo(getPluginTime(), "Path for UNIX Socket =>");
    ovPrintInfo(getPluginTime(), socketPath);
    
    setSocketPath(socketPath);

    SetPostFunction(handlePostData);

    startHTTPDServer();
    return EXIT_SUCCESS;
}
