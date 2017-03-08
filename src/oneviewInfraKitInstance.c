
// oneviewInfraKitInstance.c

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */

#include "oneviewInfraKitInstance.h"
#include "oneviewInfraKitConsole.h"
#include "oneviewInfraKitPlugin.h"
#include "oneviewInfraKitState.h"
#include "oneview.h"
#include <string.h>
#include <stdlib.h>

/* jsonrpc error return codes
 */

json_int_t parse_error = -32700;
json_int_t invald_request = -32600;
json_int_t method_not_found = -32601;
json_int_t invalid_params = -32602;
json_int_t internal_error = -32603;

/* global variable used by functions to identify if they should modify the power
 * state when attempting to apply a profile.
 */

json_t *powerState;

/* This session is for the duration of InfraKit and isn't designed to renew
 * cookies once they have expired
 */

oneviewSession *infrakitSession;

/* These function(s) handle logging into the physical infrastructure.
 *
 */

int instanceLogin(const char *address, const char *username, const char *password)
{
    infrakitSession = initSession();
    if (!infrakitSession) {
        return EXIT_FAILURE;
    }
    infrakitSession->address = strdup(address);
    infrakitSession->username = strdup(username);
    infrakitSession->password = strdup(password);
    infrakitSession->version = identifyOneview(infrakitSession);
    
    return ovLogin(infrakitSession);
}

int destroyServerProfile(const char *hardwareURI) {
    char *profileURI = serverProfileFromHardwareURI(infrakitSession, (char *) hardwareURI);
    if (profileURI) {
        // remove the server profile
        ovDeleteProfile(infrakitSession, profileURI);
    } else {
        // warning that the state lists a profile that isn't attached to hardware
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/* Find available hardware that matches the profile hardware type */

char *findFreeHardware(oneviewSession *session, const char *hardwareTypeuri)
{
    if ((session) && session->address && session->cookie) {
        
        // Get the RAW JSON return from the Server Profiles
        char *hardwareRAWJSON = NULL;
        hardwareRAWJSON = ovQueryServerHardware(session, NULL);
        
        if (hardwareRAWJSON) {
            json_t *hardwareJSON;
            json_error_t error;
            // Parse the JSON
            hardwareJSON = json_loads(hardwareRAWJSON, 0, &error);
            // If the JSON was loaded correctly attempt to parse it
            if (hardwareJSON) {
                free (hardwareRAWJSON);
                json_t *memberArray = json_object_get(hardwareJSON, "members");
                if ((memberArray) && json_array_size(memberArray) != 0) {
                    size_t memberIndex;
                    json_t *memberValue;
                    
                    json_array_foreach(memberArray, memberIndex, memberValue) {
                    const char *hardwareuri = json_string_value(json_object_get(memberValue, "serverHardwareTypeUri"));
                    const char *uri = json_string_value(json_object_get(memberValue, "uri"));
                    if (stringMatch((char *)hardwareuri, (char *)hardwareTypeuri)) {
                        // Check to see if the server appears in the state data (this is due to a delay in OneView reflecting the "Current" state)
                        if (!compareInstanceValueToKey("LogicalID", uri))  {
                            
                            // WARNING: WILL NEED REMOVING
                        
                        } else {
                            const char *assignedProfile = json_string_value(json_object_get(memberValue, "serverProfileUri"));

                            if (!assignedProfile) {
                                // Found a server that is free
                                if (json_is_true(powerState)) {
                                    const char *power = json_string_value(json_object_get(memberValue, "powerState"));
                                    if ((power) && stringMatch(power, "On")) {
                                        // Free server has been found, however its power state is "on"
                                        ovPrintInfo(getPluginTime(), "Available server being powered off, so profile can be applied\n");
                                        ovPowerOffHardware(session, uri);
                                    } else {
                                        char *availableHarware = strdup(uri);
                                        json_decref(hardwareJSON);
                                        return availableHarware;
                                    }
                                } else {
                                    char *availableHarware = strdup(uri);
                                    json_decref(hardwareJSON);
                                    return availableHarware;
                                }
                            }
                        }
                    }
                }
            }
            json_decref(hardwareJSON);
        }
    }
    }
    return NULL; // No available hardware
}

/* Iterate through all of the server profiles and find the URI that matches the profile name string */

profile *mapProfileNameToURI(oneviewSession *session, const char *profileName)
{
    if ((session) && session->address && session->cookie) {
        
        // Get the RAW JSON return from the Server Profiles
        char *profileHTTP = NULL;
        profileHTTP = ovQueryServerProfileTemplates(session, NULL);
        
        if (profileHTTP) {
            json_t *profileJSON;
            json_error_t error;
            // Parse the JSON
            profileJSON = json_loads(profileHTTP, 0, &error);
            // If the JSON was loaded correctly attempt to parse it
            if (profileJSON) {
                // Free the raw JSON as it has been processed
                free(profileHTTP);
                
                json_t *memberArray = json_object_get(profileJSON, "members");
                if ((memberArray) && json_array_size(memberArray) != 0) {
                    
                    size_t memberIndex;
                    json_t *memberValue;
                    
                    json_array_foreach(memberArray, memberIndex, memberValue) {
                        // retrieve needed statistics
                        char *name = (char *)json_string_value(json_object_get(memberValue, "name"));
                        if (stringMatch((char *)profileName, name)) {
                            const char *uri = json_string_value(json_object_get(memberValue, "uri"));
                            const char *hardwareuri = json_string_value(json_object_get(memberValue, "serverHardwareTypeUri"));
                            const char *enclosureuri = json_string_value(json_object_get(memberValue, "enclosureGroupUri"));
                            
                            if (uri && hardwareuri && enclosureuri) {
                                // There should always be the uris if there has been a name object, otherwise something
                                // is internally broken inside of OneView
                                
                                // all of the strings passed into the match struct will need freeing.
                                
                                // Check if hardware is available before building rest of new profile
                                char *availHWURI = findFreeHardware(session, hardwareuri);
                                if (!availHWURI) {
                                    ovPrintError(getPluginTime(), "No Server Hardware is available\n");
                                    json_decref(profileJSON);
                                    return NULL;
                                }
                                profile *match = malloc(sizeof(profile));
                                if (match) {
                                    match->enclosureUri = strdup(enclosureuri);
                                    match->templateName = strdup(name);
                                    match->hardwareTypeUri = strdup(hardwareuri);
                                    match->uri = strdup(uri);
                                    match->availableHardwareURI = availHWURI;
                                    json_decref(profileJSON);
                                    return match;
                                }
                            }
                        }
                    }
                }
                json_decref(profileJSON);
            }
        }
    }
    return NULL;
}

instance *processInstanceJSON(json_t *paramsJSON, long long id)
{
    // If the JSON was loaded correctly attempt to parse it
    if (paramsJSON) {
        // Check for the properties object
        json_t *properties = json_object_get(paramsJSON, "Properties");
        json_t *ovCredentials = json_object_get(properties, "OneView");
        // Check for the session details first as these are required for interacting with OneView
        const char *address = getenv("OV_ADDRESS");
        const char *username = getenv("OV_USERNAME");
        const char *password = getenv("OV_PASSWORD");

        if (ovCredentials) {
            if (!address) {
                ovPrintInfo(getPluginTime(), "Environment variable OV_ADDRESS not set, looking in JSON config\n");
                address = json_string_value(json_object_get(ovCredentials, "OneViewAddress"));
            }
            if (!username) {
                ovPrintWarning(getPluginTime(), "Environment variable OV_USERNAME not set, looking in JSON config\n");
                username = json_string_value(json_object_get(ovCredentials, "OneViewUsername"));
            }
            if (!password) {
                ovPrintWarning(getPluginTime(), "Environment variable OV_PASSWORD not set, looking in JSON config\n");
                password = json_string_value(json_object_get(ovCredentials, "OneViewPassword"));
            }
        }
        // ensure none of these values are NULL before attempting to log in

        if (address && username && password) {
            if (instanceLogin(address, username, password) == EXIT_FAILURE) {
                ovPrintError(getPluginTime(), "Login Failed\n");
                return NULL;
            }
        } else {
            ovPrintError(getPluginTime(), "No Credentials supplied to OneView\n");
            return NULL;
        }
        
        if (properties && infrakitSession) {
            if (!infrakitSession->cookie) {
                ovPrintError(getPluginTime(), "OneView session not found\n");
                return NULL;
            }
            
            /*************
              Work through the server profile creation
             *************/
            
            
            // Name of the HPE OneView Profile to be used
            const char *templateName = json_string_value(json_object_get(properties, "TemplateName"));
            // Name to use when creating the server profile
            const char *profileName = json_string_value(json_object_get(properties, "ProfileName"));
            profile *foundServer = NULL;
            
            // Check to see if powerState variable is set (NULL if not set)
            powerState = json_object_get(properties, "PowerOff");

            if (templateName) {
                 foundServer = mapProfileNameToURI(infrakitSession, templateName);
                if (!foundServer) {
                    ovPrintError(getPluginTime(), "Available Hardware could not be found\n");
                    return NULL;
                }

                if (profileName) {
                    size_t profileNameLength = strlen(profileName);
                    char newName[profileNameLength+100];
                    sprintf(newName, "%s-%llu", profileName, id);
                    foundServer->profileName = strdup(newName);
                } else {
                    size_t profileNameLength = strlen(templateName);
                    char newName[profileNameLength+100];
                    sprintf(newName, "%s-%llu", templateName, id);
                    foundServer->profileName = strdup(newName);
                }
                char ovOutput[1024];
                sprintf(ovOutput, "Creating Instance => %s\n", foundServer->profileName);
                ovPrintInfo(getPluginTime(), ovOutput);
                
                char *newProfile = ovQueryNewServerProfileTemplates(infrakitSession, NULL, foundServer->uri);
                if (newProfile) {
                    json_t *newProfileJSON;
                    json_error_t error;
                    // Parse the JSON
                    newProfileJSON = json_loads(newProfile, 0, &error);
                    if (newProfileJSON) {
                        json_object_set(newProfileJSON, "name", json_string(foundServer->profileName));
                        json_object_set(newProfileJSON, "serverHardwareUri", json_string(foundServer->availableHardwareURI));
                        json_object_set(newProfileJSON, "description", json_string(getStatePath()));
                        char *rawProfileJSON = json_dumps(newProfileJSON, JSON_ENSURE_ASCII);
                        
                        
                        if (rawProfileJSON) {
                            ovPostProfile(infrakitSession, rawProfileJSON);
                            appendInstanceToState(foundServer, infrakitSession, paramsJSON);
                            json_decref(newProfileJSON);
                            
                            instance *newInstance = malloc(sizeof(instance));
                            newInstance->instanceName = strdup(foundServer->profileName);
                            newInstance->instanceType = INSTANCE_SERVER_PROFILE;
                            freeServerProfile(foundServer);
                            free(rawProfileJSON);
                            return newInstance;
                        }
                        json_decref(newProfileJSON);
                    }
                }
                
            }
            
            //const char *serverLocation = json_string_value(json_object_get(properties, "ServerLocation"));
            
            
            /* This will take either the template name or an assigned server profile name, 
             and it will then append the unique ID to the end of the name to be used as a new profile name.
             e.g.   TEMPLATE-NAME={123425436547-1234325436547-1243253}
             */
            

            /*************
             Work through the Network profile creation
             *************/
            
            json_t *networksConfiguration = json_object_get(properties, "Networks");
            if (networksConfiguration) {
                json_t *uniqueNetworkName = json_object_get(properties, "Unique");
                if (json_is_true(uniqueNetworkName)) {
                    size_t networkNameLength = strlen(templateName);
                    char newName[networkNameLength+100];
                    sprintf(newName, "%s-%llu", templateName, id);
                }
                
                json_t *networksConfigArray = json_object_get(properties, "NetworkConfig");
                
                if (networksConfigArray && (json_array_size(networksConfigArray) != 0)) {
                
                    size_t networkIndex;  // Array Index value
                    json_t *networkValue; // Current Network Object
                    /*
                     { "networkName" : "name",
                        "networkVlan" : <VlanID> }
                     */
                
                    json_array_foreach(networksConfigArray, networkIndex, networkValue) {
                        // Identify the Key/Value pairs needed to build our networks
                        const char *networkName = json_string_value(json_object_get(networkValue, "networkName"));
                        const char *networkVlan = json_string_value(json_object_get(networkValue, "networkVlan"));
                        // Check that both aren't NUL in which case there is a configuration error
                        if (networkName && networkVlan) {
                            // We have a network name/vlan, lets create
                        
                            ovQueryNetworks(infrakitSession, NULL);
                        
                        } else {
                            ovPrintError(getPluginTime(), "Error with commited networking configuration\n");
                        }
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

/* Evaluate the struct and determine what is populated
   then free resources back to the heap.
 */

int freeServerProfile(profile *freeProfile)
{
    if (freeProfile) {
        if (freeProfile->availableHardwareURI) {
            free(freeProfile->availableHardwareURI);
        }
        if (freeProfile->enclosureUri) {
            free(freeProfile->enclosureUri);
        }
        if (freeProfile->hardwareTypeUri) {
            free(freeProfile->hardwareTypeUri);
        }
        if (freeProfile->profileName) {
            free(freeProfile->profileName);
        }
        if (freeProfile->templateName) {
            free(freeProfile->templateName);
        }
        if (freeProfile->uri) {
            free(freeProfile->uri);
        }
        free(freeProfile);
        freeProfile = NULL;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/* Evaluate the struct and determine what is populated
 then free resources back to the heap.
 */

int freeNetwork(network *freeNetworkInstance)
{
    if (freeNetworkInstance) {
        if (freeNetworkInstance->networkName) {
            free(freeNetworkInstance->networkName);
        }
        free(freeNetworkInstance);
        freeNetworkInstance = NULL;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/* Evaluate the struct and determine what is populated
 then free resources back to the heap.
 */

int freeInstance(instance *freeInstance)
{
    if (freeInstance) {
        if (freeInstance->instanceName) {
            free(freeInstance->instanceName);
        }
        free(freeInstance);
        freeInstance = NULL;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/* Check through the state file and compare the physical state
 * then update the state file so that InfraKit is kept current with
 * the physical Infrastructure state.
 */

int synchroniseStateWithPhysical()
{
    
    /* 
     Investigate Server Hardware -> State
     */

    if (loginFromState() == EXIT_SUCCESS) {
        json_t *StateJSON = openInstanceState("");
        json_t *previousInstances = json_object_get(StateJSON, "Instances");
        json_t *previousNonFunctional = json_object_get(StateJSON, "NonFunctional");
        
        json_t *currentInstances = json_array();
        json_t *currentNonFunctional = json_array();
       
        // Iterate over the non-functional instances
        
        size_t memberIndex;
        json_t *memberValue;
        
        /*  If one member is part of both it will compound the problem...
         *
         */
        
        json_array_foreach(previousNonFunctional, memberIndex, memberValue) {
            const char *hardwareURI = json_string_value(json_object_get(memberValue, "LogicalID"));
            if (hardwareURI) {
                char *profileURI = serverProfileFromHardwareURI(infrakitSession, (char *) hardwareURI);
                if (profileURI) {
                    // Check power state and add to active / non-functional
                    json_array_append(currentInstances, memberValue);
                    //json_array_append(currentNonFunctional, memberValue);
                }
            }
        }
        
        
        json_array_foreach(previousInstances, memberIndex, memberValue) {
            const char *hardwareURI = json_string_value(json_object_get(memberValue, "LogicalID"));
            json_t *tags = json_object_get(memberValue, "Tags");
            const char *counterString = json_string_value(json_object_get(tags, "retry-count"));
            char *endPointer;
            long retry_counter = strtol(counterString, &endPointer, 10);
            char debugString[1024];
            
            snprintf(debugString, 1024, "HW = %s Remaining = %zu\n", hardwareURI, retry_counter);
            ovPrintDebug(getPluginTime(), debugString);

            if (hardwareURI) {
                char *profileURI = serverProfileFromHardwareURI(infrakitSession, hardwareURI);
                char *state = stateFromHardwareURI(infrakitSession, hardwareURI);

                if (profileURI) {

                    //If the profile has been applied, remove the retry-count
                    if (stringMatch(state, "ProfileApplied")) {
                        json_string_set(json_object_get(tags, "retry-count"), "0");
                    } else {
                        ovPrintDebug(getPluginTime(), "Still applying Server Profile\n");
                    }
                    
                    json_array_append(currentInstances, memberValue);

                } else if (retry_counter > 0) {
                    retry_counter--;
                    char buf[retry_counter+1];
                    snprintf(buf, retry_counter+1, "%ld", retry_counter);
                    json_string_set(json_object_get(tags, "retry-count"), buf);
                    json_array_append(currentInstances, memberValue);
                }
            }
        }
        
        // Two updated new arrays to replace inside our state
        json_object_set(StateJSON, "Instances", currentInstances);
        json_object_set(StateJSON, "NonFunctional", currentNonFunctional);

        char *json_text = json_dumps(StateJSON, JSON_ENSURE_ASCII);
        saveInstanceState(json_text);
        free(json_text);
        json_decref(StateJSON);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}


 /*******************************************************************************************/

 /* Docker InfraKit methods, these methods are called directly from the HTTPD server and
  * provide the capability for provisioning, monitoring and destroying of infrastructure.
  *
  */


 /* ovInfraKitInstanceProvision(json_t *params, long long id)
  * params = Parameter JSON that the instance uses for configuration
  * id = method call id, to ensure function sycnronisation
  *
  * Unlike the other methods, this will use the credentials in the parameters to configure
  *
  */


char *ovInfraKitInstanceProvision(json_t *params, long long id)
{
    instance *newInstance = processInstanceJSON(params, id);
    
    char *successProvisionResponse = "{s:s,s:{s:s?},s:I}";
    char *failProvisionResponse = "{s:s,s:{s:i},s:I}";

    char *response;
    json_t *responseJSON;
    if (newInstance) {
        responseJSON = json_pack(successProvisionResponse,  "jsonrpc", "2.0",                   \
                                                            "result",                           \
                                                                "ID", newInstance->instanceName,\
                                                            "id", id);
    } else {
        responseJSON = json_pack(failProvisionResponse,     "jsonrpc", "2.0",                   \
                                                            "error",                            \
                                                                "code", parse_error,            \
                                                            "id", id);
    }
    response = json_dumps(responseJSON, JSON_ENSURE_ASCII);
    json_decref(responseJSON);
    return response;
}

/* ovInfraKitInstanceDescribe(json_t *params, long long id)
 * params = Parameter JSON that the instance uses for configuration
 * id = method call id, to ensure function sycnronisation
 *
 * This function should use the plugin-state, defined in a file and compare it with
 * the state of the infrastructure we're hoping to configure.
 */

char *ovInfraKitInstanceDescribe(json_t *params, long long id)
{
    if (synchroniseStateWithPhysical() == EXIT_FAILURE) {
        ovPrintWarning(getPluginTime(), "Failed to synchronise state");
    }
    json_t *instanceState = openInstanceState("");
    json_t *instanceArray = json_object_get(instanceState, "Instances");
    char *DescriptionResponse = "{s:s,s:{s:[]},s:s?,s:I}";
    json_t *responseJSON = json_pack(DescriptionResponse,   "jsonrpc", "2.0",                   \
                                                            "result",                           \
                                                                "Descriptions",                 \
                                                            "error", NULL,                      \
                                                            "id", id);
    json_t *array = json_object_get(responseJSON, "result");
    json_object_set(array, "Descriptions", instanceArray);
    char *response = json_dumps(responseJSON, JSON_ENSURE_ASCII);
    json_decref(responseJSON);
    return response;
}

char *ovInfraKitInstanceDestroy(json_t *params, long long id)
{
    
    int InstanceRemoved;

    /* The InstanceID is the Hardware URI, which is the unique identifier for a
     * physical server inside of HPE OneView
     */
    
    const char *instanceID = json_string_value(json_object_get(params, "Instance"));
    
    
    char *physicalID = returnValueFromInstanceKey(instanceID, "LogicalID");

    if (loginFromState() == EXIT_SUCCESS) {
        if (destroyServerProfile(physicalID) == EXIT_SUCCESS) {
            if (instanceID) {
                InstanceRemoved = removeInstanceFromState(instanceID);
            }
        } else {
            ovPrintError(getPluginTime(), "Failed to remove instance =>\n");
            if (instanceID){
                ovPrintError(getPluginTime(), instanceID);
                ovPrintError(getPluginTime(), "\n");

            }
        }
    } else {
        ovPrintError(getPluginTime(), "Error connecting to HPE OneView\n");
        return NULL;
    }
    char *successProvisionResponse = "{s:s,s:{s:s?},s:I}";
    char *failProvisionResponse = "{s:s,s:{s:i},s:I}";
    char *response;
    json_t *responseJSON;
    if (InstanceRemoved == EXIT_SUCCESS) {
        // Announce the server profile being removed
        char ovOutput[1024];
        sprintf(ovOutput, "Removing Instance => %s\n", instanceID);
        ovPrintInfo(getPluginTime(), ovOutput);
        
        responseJSON = json_pack(successProvisionResponse,  "jsonrpc", "2.0",                   \
                                                            "result",                           \
                                                                "Instance", instanceID,         \
                                                            "id", id);
    } else {
        responseJSON = json_pack(failProvisionResponse,     "jsonrpc", "2.0",                   \
                                                            "error",                            \
                                                                "code", parse_error,            \
                                                            "id", id);
    }
    response = json_dumps(responseJSON, JSON_ENSURE_ASCII);
    json_decref(responseJSON);
    return response;

}

