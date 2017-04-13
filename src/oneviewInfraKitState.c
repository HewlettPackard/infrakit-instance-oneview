
// oneviewInfraKitState.c

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */


#include "oneviewInfraKitState.h"
#include "oneviewInfraKitPlugin.h"
#include "oneviewInfraKitConsole.h"

#include <string.h>

char *statePath = NULL;
char *argStatePath = NULL;

char *getStatePath()
{
    return statePath;
}

int setStatePath(char *path)
{
    if (path && (strlen(path) > 1)) {
        free(statePath);
        statePath = strdup(path);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

char *getArgStatePath()
{
    return argStatePath;
}

int setArgStatePath(char *path)
{
    if (path && (strlen(path) > 1)) {
        free(statePath);
        statePath = strdup(path);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}


/* The Retry count determins how many times the Instance.Describe will check that a
 * server profile is being created, once the counter reaches 0 a new instance will be 
 * provisioned. 
 *
 * Set to 30 by default for created more than 30 server profiles at once.
 *
 * THIS HAS TO BE A STRING VALUE AS DEFINED BY INFRAKIT
 *
 */

#define INSTANCE_RETRY "5"

/* This will read through the state data, and attempt to find the OneView Credentials to allow
 * other functions to login to OneView and speak to the API.
 */

int loginFromState(const char *groupName)
{
    if (!groupName) {
        ovPrintError(getPluginTime(), "No Group could be found for state data\n");
        return EXIT_FAILURE;
    }
    json_t *stateJSON = openInstanceState();
    json_t *group = findGroup(stateJSON, groupName);
    json_t *oneViewState = json_object_get(group, "OneViewInstance");
    
    if ((oneViewState) && json_object_size(oneViewState) != 0) {
        const char *address = json_string_value(json_object_get(oneViewState, "address"));
        const char *username = json_string_value(json_object_get(oneViewState, "username"));
        const char *password = json_string_value(json_object_get(oneViewState, "password"));
        return instanceLogin(address, username, password);
    }
    return EXIT_FAILURE;
}


 /* These functions will open a file at a specified path, and if  found attempt to read
  * the JSON. If the file isn't found or the JSON wont load, it will create a new state.
  *
  */

json_t *openInstanceState()
{
    if (!statePath) {
        ovPrintError(getPluginTime(), "No path specified for state data\n");
        return NULL;
    }
    json_t *stateJSON;
    json_error_t error;
    stateJSON = json_load_file(statePath, 0, &error);
    if (!stateJSON) {
        stateJSON = json_pack("{s:s,s:[]}", "StateVersion", "0.3.0" , "OneViewGroups");
    }
    return stateJSON;
}

int saveInstanceState(char *jsonData)
{
    if (!statePath) {
        ovPrintError(getPluginTime(), "No path specified for state data\n");
        return EXIT_FAILURE;
    }
    
    FILE *fp;
    fp = fopen(statePath, "w");
    if (fp) { /*file opened succesfully */
        fputs(jsonData, fp);
        fclose(fp);
    } else {
        ovPrintError(getPluginTime(), "Unable to modify the state file =>\n");
        ovPrintError(getPluginTime(), statePath);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/* This function will take the state data, and then find a group
 * within that data, if found it will return the JSON object attached
 * to that group
 */

json_t *findGroup(json_t *state, const char *groupName)
{
    if (state && groupName) {
        size_t groupIndex;
        json_t *group;
        json_t *oneViewGroups = json_object_get(state, "OneViewGroups");
        json_array_foreach(oneViewGroups, groupIndex, group) {
            const char *currentGroup = json_string_value(json_object_get(group, "groupName"));
            if (stringMatch(currentGroup, groupName)) {
                return group;
            }
        }
    }
    return NULL;
}

/* This function will take a new profile, and attach the various details to the state data
 * HardwareURI, InstanceID (profile Name), it will also place the OneView credentials in the
 * state file so that other functions can find them and use them to confirm the state.
 */

int appendInstanceToState(profile *foundServer, oneviewSession *session, json_t *paramsJSON)
{
    json_t *tags = json_object_get(paramsJSON, "Tags");
    const char *sha = json_string_value(json_object_get(tags, "infrakit.config_sha"));
    const char *infrakitGroup = json_string_value(json_object_get(tags, "infrakit.group"));
    
    json_t *stateJSON = openInstanceState();
    if (!stateJSON) {
        ovPrintError(getPluginTime(), "Unable to preserve state\n");
        return EXIT_FAILURE;
    }
    
    char ovOutput[1024];
    sprintf(ovOutput, "Opening State for Group %s\n", infrakitGroup);
    ovPrintDebug(getPluginTime(), ovOutput);
    
    json_t *oneViewGroups = json_object_get(stateJSON, "OneViewGroups");
    json_t *group = findGroup(stateJSON, infrakitGroup);

    if (!group) {
       group = json_pack ("{s:s,s:{},s:[],s:[]}", "groupName", infrakitGroup, "OneViewInstance", "Instances", "NonFunctional");
       json_array_append(oneViewGroups, group);
    }

    json_t *instances = json_object_get(group, "Instances");
    if (json_object_size(json_object_get(group, "OneViewInstance")) == 0) {
        char *oneviewDetails = "{s:s?,s:s?,s:s?,s:s?}";
        json_t *oneviewJSON = json_pack(oneviewDetails, \
                                            "address", session->address, \
                                            "username", session->username, \
                                            "password", session->password, \
                                            "cookie", session->cookie);
        
        json_object_set(group, "OneViewInstance", oneviewJSON);
    }

    char *instanceDescription = "{s:s,s:s?,s:{s:s,s:s,s:s,s:s}}";
    json_t *descriptionJSON = json_pack(instanceDescription, \
                                            "ID", foundServer->profileName, \
                                            "LogicalID", foundServer->availableHardwareURI, \
                                            "Tags", \
                                                "hw_uri", foundServer->availableHardwareURI, \
                                                "retry-count", INSTANCE_RETRY, \
                                                "infrakit.config_sha", sha, \
                                                "infrakit.group", infrakitGroup);
    
    json_array_append(instances, descriptionJSON);
    char *json_text = json_dumps(stateJSON, JSON_ENSURE_ASCII);
    saveInstanceState(json_text);
    free(json_text);
    json_decref(stateJSON);
    return EXIT_SUCCESS;
}


/* This function will iterate through the instance state and remove an instance, 
 * by using it's instanceID
 */

int removeInstanceFromState(const char *instanceID, const char *groupName)
{
    
    json_t *stateJSON = openInstanceState();
    json_t *group = findGroup(stateJSON, groupName);
    json_t *instances = json_object_get(group, "Instances");
    
    /* No Instance to compare, so failure to match
     */
    
    if (json_array_size(instances) == 0) {
        return EXIT_FAILURE;
    }
    
    if (instances) {
        size_t instanceIndex;
        json_t *instanceValue;
        signed long instanceLocation = -1;
        json_array_foreach(instances, instanceIndex, instanceValue) {
            const char *currentValue = json_string_value(json_object_get(instanceValue, "ID"));
            if (currentValue) {
                if (stringMatch((char *)currentValue, (char *)instanceID)) {
                    instanceLocation = instanceIndex;
                }
            }
        }
        // See if instance was found in array
        if (instanceLocation != -1) {
            json_array_remove(instances, instanceLocation);
            char *json_text = json_dumps(stateJSON, JSON_ENSURE_ASCII);
            saveInstanceState(json_text);
            free(json_text);
            json_decref(stateJSON);
            return EXIT_SUCCESS;
        }
    }
    json_decref(stateJSON);
    return EXIT_FAILURE;
}

/* These functions query the state of Instances, or can return values assigned to
 * instances.
 */

json_t *returnObjectFromInstanceID(const char *InstanceID)
{
    json_t *state = openInstanceState();
    if (!state) {
        return NULL;
    }
    
    if (state) {
        size_t groupIndex;
        json_t *group;
        json_t *oneViewGroups = json_object_get(state, "OneViewGroups");
        json_array_foreach(oneViewGroups, groupIndex, group) {

            json_t *instances = json_object_get(group, "Instances");
            
            /* No Instance to compare, so failure to match
             */
//            if (json_array_size(instances) == 0) {
//                return NULL;
//            }
            
            if (instances) {
                size_t instanceIndex;
                json_t *instanceValue;
                
                json_array_foreach(instances, instanceIndex, instanceValue) {
                    const char *ID = json_string_value(json_object_get(instanceValue, "ID"));
                    if (stringMatch((char *)ID, InstanceID)) {
                        json_t *returnObject = json_copy(instanceValue);
                        json_decref(state);
                        return returnObject;
                    }
                }
            }
        }
    }
    return NULL;
}


char *returnInstanceFromState(const char *InstanceID, char *key)
{
    json_t *state = openInstanceState();
    if (!state) {
        return NULL;
    }
    
    if (state) {
        size_t groupIndex;
        json_t *group;
        json_t *oneViewGroups = json_object_get(state, "OneViewGroups");
        json_array_foreach(oneViewGroups, groupIndex, group) {
            
            json_t *instances = json_object_get(group, "Instances");
            
            /* No Instance to compare, so failure to match
             */
//            if (json_array_size(instances) == 0) {
//                return NULL;
//            }
            
            if (instances) {
                size_t instanceIndex;
                json_t *instanceValue;
                
                json_array_foreach(instances, instanceIndex, instanceValue) {
                    const char *ID = json_string_value(json_object_get(instanceValue, "ID"));
                    if (stringMatch((char *)ID, InstanceID)) {
                        // found our instance
                        const char *value = json_string_value(json_object_get(instanceValue, key));
                        if (value) {
                            // Key exists and has returned a value
                            char *returnValue = strdup(value);
                            json_decref(state);
                            return returnValue;
                        }
                    }
                }
            }
        }
    }
    // Not found in our state
    json_decref(state);
    return NULL;
}

/* This will iterate through the state file and determine
 * if the hardware URI is already applied to a server profile
 * This is because the plugin moves faster than OneView does.
 */

int findUsedHWInState(const char *hardwareURI)
{
    json_t *state = openInstanceState();
    if (!state) {
        ovPrintError(getPluginTime(), "Can't read State Data\n");
        return EXIT_FAILURE;
    }
    
    if (state) {
        size_t groupIndex;
        json_t *group;
        json_t *oneViewGroups = json_object_get(state, "OneViewGroups");
        json_array_foreach(oneViewGroups, groupIndex, group) {
            
            json_t *instances = json_object_get(group, "Instances");
            
            /* No Instance to compare, so failure to match
             */
//            if (json_array_size(instances) == 0) {
//                ovPrintInfo(getPluginTime(), "No instances or Hardware allocated\n");
//                return EXIT_SUCCESS;
//            }
//            
            if (instances) {
                size_t instanceIndex;
                json_t *instanceValue;
                
                json_array_foreach(instances, instanceIndex, instanceValue) {
                    const char *ID = json_string_value(json_object_get(instanceValue, "ID"));
                    json_t *tags = json_object_get(instanceValue, "Tags");
                    const char *stateURI = json_string_value(json_object_get(tags, "hw_uri"));
                    if (stringMatch(stateURI,hardwareURI)) {
                        // Found the hardware URI applied to a server profile
                        char ovOutput[1024];
                        snprintf(ovOutput, 1024, "Hardware belongs to %s\n", ID);
                        ovPrintDebug(getPluginTime(), ovOutput);
                        json_decref(state);
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }
    // Not found in our state
    json_decref(state);
    return EXIT_SUCCESS;
}

 /*  In the event a describe is done directly to the plugin, then a group wont be specified
  *  For this will take ALL instances from ALL groups and compile a full list of instances
  *  that the plugin is managing.
  */

json_t *returnAllInstances(json_t *state)
{
    
    return NULL;
}
