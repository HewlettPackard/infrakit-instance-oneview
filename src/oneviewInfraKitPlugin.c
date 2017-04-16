
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
#include "oneviewHTTPD.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* Added in OS specific includes to allow compilation on Linux / OSX
 */

#if defined(__linux__)
#include <linux/limits.h>
#endif

#if defined(__APPLE__)
#include <limits.h>
#endif

#include "jansson.h"

char *argumentSocketPath;
char *argumentLogLevel;

/* These variables hold the defaults 
 * or the user specified names of the 
 * sockets and the state files
 */

char *socketName = NULL;
char *stateName = NULL;

char *socketDefault = "instance-oneview";
char *stateDefault = "state-oneview.json";

// These will be built up to the fullpaths are runtime
char builtSocketPath[PATH_MAX];
char builtStatePath[PATH_MAX];

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

int handlePostData(httpRequest *request)
{
    json_t *requestJSON = NULL;
    json_error_t error;
    
    requestJSON = json_loads(request->messageBody, 0, &error);
    if (requestJSON) {
        const char *methodName = json_string_value(json_object_get(requestJSON, "method"));
        long long id = json_integer_value(json_object_get(requestJSON, "id"));
        json_t *params = json_object_get(requestJSON, "params");
        
        if (getConsoleOutputLevel() == LOGDEBUG) {
            char *debugMessage = json_dumps(requestJSON, JSON_INDENT(3));
            ovPrintDebug(getPluginTime(), "Incoming Request =>\n");
            ovPrintDebug(getPluginTime(), debugMessage);
            free(debugMessage);
        }
        
        if (stringMatch(methodName, "Instance.DescribeInstances")) {
            char *response = ovInfraKitInstanceDescribe(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>\n");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        if (stringMatch(methodName, "Handshake.Implements")) {
            json_t *reponseJSON = json_pack("{s:s,s:{s:[{s:s,s:s}]},s:I}", "jsonrpc", "2.0", "result", "APIs", "Name", "Instance", "Version", "0.5.0", "id", id);
            char *response = json_dumps(reponseJSON, JSON_ENSURE_ASCII);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        // Backwards compatability (should be removed in the future)
        if (stringMatch(methodName, "Plugin.Implements")) {
            json_t *reponseJSON = json_pack("{s:s,s:{s:[{s:s,s:s}]},s:I}", "jsonrpc", "2.0", "result", "APIs", "Name", "Instance", "Version", "0.1.0", "id", id);
            char *response = json_dumps(reponseJSON, JSON_ENSURE_ASCII);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        if (stringMatch(methodName, "Instance.Validate")) {
            json_t *reponseJSON = json_pack("{s:{s:b},s:s?,s:I}", "result", "OK", JSON_TRUE, "error", NULL, "id", id);
            char *response = json_dumps(reponseJSON, JSON_ENSURE_ASCII);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        
        if (stringMatch(methodName, "Instance.Provision")) {
            json_t *spec = json_object_get(params, "Spec");
            char *response = ovInfraKitInstanceProvision(spec, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>\n");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        if (stringMatch(methodName, "Instance.Destroy")) {
            char *response = ovInfraKitInstanceDestroy(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>\n");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        if (stringMatch(methodName, "Instance.Meta")) {
            char *response = ovInfraKitInstanceDestroy(params, id);
            ovPrintDebug(getPluginTime(), "Outgoing Response =>\n");
            ovPrintDebug(getPluginTime(), response);
            setHTTPResponse(response, 200);
            json_decref(requestJSON);
            return EXIT_SUCCESS;
        }
        if (methodName) {
            ovPrintError(getPluginTime(), "Unknown JSON-RPC Method\n");
        }
        json_decref(requestJSON);
        return EXIT_FAILURE;
    }
    ovPrintError(getPluginTime(), "Can't process HTTPD POST\n");
    return EXIT_FAILURE;
}

void handleInterrupt(int signal){
    char pluginOutput[1024];
    sprintf(pluginOutput, "Exit Signal: %d, Removing Socket: %s\n", signal, builtSocketPath);
    ovPrintWarning(getPluginTime(), pluginOutput);
    unlink(builtSocketPath);
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
    ovPrintInfo(getPluginTime(), "Starting OneView Instance Plugin\n");
    
    /* These two paths will build out to be the path for the socket and the state
     * we will build them out and ensure that the paths are fully created, including
     * the directory paths.
     */
    
    sprintf(builtSocketPath, "%s/", getenv("HOME"));
    strcat(builtSocketPath, ".infrakit/");
    int response = 0;
    response = mkdir(builtSocketPath, 0755);
    if ((response == -1) && errno != EEXIST ) {
        ovPrintError(getPluginTime(), "Error creating ~/.InfraKit Plugin can not start\n");
        return EXIT_FAILURE;
    }
    
    
    /* Look through the parameters to determing the group ID, this is
     * needed for multi-tenancy groups. The problem here is that the instance.destroy
     * doesn't have a group tag
     */
    
//    const char *groupName = json_string_value(json_object_get(json_object_get(params, "Tags"), "infrakit.group"));
//    if (!groupName) {
//        json_t *spec = json_object_get(params, "Spec");
//        groupName = json_string_value(json_object_get(json_object_get(spec, "Tags"), "infrakit.group"));
//    }
    

    if (getArgStatePath()) {
        setStatePath(getArgStatePath());
    } else {
        sprintf(builtStatePath, "%sstate/", builtSocketPath);
        response = mkdir(builtStatePath, 0755);
        if ((response == -1) && errno != EEXIST ) {
            ovPrintError(getPluginTime(), "Error creating ~/.InfraKit/state Plugin can not start\n");
            return EXIT_FAILURE;
        }
        strcat(builtStatePath, stateDefault);
        setStatePath(builtStatePath);
    }

    
    
    strcat(builtSocketPath, "plugins/");
    response = mkdir(builtSocketPath, 0755);
    if ((response == -1) && errno != EEXIST ) {
        ovPrintError(getPluginTime(), "Error creating ~/.InfraKit/plugins Plugin can not start\n");
        return EXIT_FAILURE;
    }
    
    if (socketName) {
        strcat(builtSocketPath, socketName);
    } else {
        strcat(builtSocketPath, socketDefault);
    }
    setSocketPath(builtSocketPath);

    char ovOutput[1024];
    sprintf(ovOutput, "Path for Unix Socket => %s\n", builtSocketPath);
    ovPrintInfo(getPluginTime(), ovOutput);
    sprintf(ovOutput, "Path for State File => %s\n", builtStatePath);
    ovPrintInfo(getPluginTime(), ovOutput);

    SetPostFunction(handlePostData);

    startHTTPDServer();
    return EXIT_SUCCESS;
}
