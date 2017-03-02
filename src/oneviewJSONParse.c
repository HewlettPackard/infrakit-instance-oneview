
// oneviewJSONParse.c

/*
 *
 * instance-infrakit-oneview - A Library for interacting with HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */



#include "oneviewJSONParse.h"
#include "oneviewInfraKitState.h"

#ifdef JSON_H
// Ensure that we're going to be using libjansson

#include <jansson.h>
#include <string.h>

json_t *json_singleton; // Shared JSON


struct interconnectLookup
{
    const char *interconnectName;
    json_t *interconnectJSON;
    json_t *portStatisticsJSON;
};

/*
 * Character based operations.
 */

char *returnRawFromJson()
{
    if (json_singleton) {
        char *rawText = json_dumps(json_singleton, JSON_ENSURE_ASCII);
        if (rawText) {
            return rawText;
        }
    }
    return NULL;
}

char *returnReadableFromJson()
{
    if (json_singleton) {
        char *rawText = json_dumps(json_singleton, JSON_INDENT(4));
        if (rawText) {
            return rawText;
        }
    }
    return NULL;
}



int setJSONfromRAW(char *raw)
{
    // Check if already allocated.. as creating a new allocation will be a mem leak
    if (json_singleton)
        return 2;
    
    if (raw) {
        json_error_t error; // Used as a passback for error data during json processing
        // Parse the JSON
        json_singleton = json_loads(raw, 0, &error);
        // If the JSON was loaded correctly return success
        if (json_singleton)
            return 1;
    }
    return 0;
}

void clearJSON()
{
    if (json_singleton)
    json_decref(json_singleton);
}



char *returnRawJSONFromObject(char* rawJSON, char *objectName)
{
    if (rawJSON) {
        
        json_t *root; // Contains all of the json data once processed by jansson
        json_error_t error; // Used as a passback for error data during json processing
        
        // Parse the JSON
        root = json_loads(rawJSON, 0, &error);
        // If the JSON was loaded correctly attempt to parse it
        if (root) {
            json_t *jsonObject = json_object_get(root, objectName);
            char *json_text = NULL;
            if (jsonObject) {
                json_incref(root); // Borrowed reference to jsonObject
                // Dump the json if found
                json_text = json_dumps(jsonObject, JSON_ENSURE_ASCII);
                // Free all references
                json_decref(jsonObject);
                json_decref(root);
            }
            if (json_text)
                return json_text;
        }
    }
    return NULL;
}

char *returnReadableJSONFromRaw(char *rawJSON)
{
    if (rawJSON) {
        json_t *root; // Contains all of the json data once processed by jansson
        json_error_t error; // Used as a passback for error data during json processing
        
        // Parse the JSON
        root = json_loads(rawJSON, 0, &error);
        if (root) {
            char *json_text = json_dumps(root, JSON_INDENT(4));
            // free the original JSON
            json_decref(root);
            // return the raw JSON
            if (json_text)
                return json_text;
        }
    }
    return NULL;
}

void ovParseJSONWithObjectName(char *arrayName, char *rawJSON)
{
    
}

void ovParseMembers(char *rawJSON, char delimiter, char *fields[], int fieldCount)
{
     ovParseArray(rawJSON, "members",  delimiter, fields,  fieldCount);
}


void ovParseArray(char *rawJSON, char *arrayName, char delimiter, char *fields[], int fieldCount)
{
    // Ensure that some fields to search through exist before trying to access them
    // WARNING, if fieldcount is more than fields[] (array size) then unexpected behaviour will occur
    if (fields) {
        json_t *root; // Contains all of the json data once processed by jansson
        json_error_t error; // Used as a passback for error data during json processing
    
        root = json_loads(rawJSON, 0, &error);
    
        json_t *memberArray = json_object_get(root, arrayName);
    
        if (json_array_size(memberArray) != 0) {
            size_t index;
            json_t *value;
            json_array_foreach(memberArray, index, value) {
                for (int i = 0; i< fieldCount; i++) {
                    if (json_is_number(json_object_get(value, fields[i]))) {
                        printf("%lli", json_integer_value(json_object_get(value, fields[i])));
                        if (i != (fieldCount - 1))
                            printf("%c", delimiter);
                    } else if (json_is_string(json_object_get(value, fields[i]))) {
                        printf("%s", json_string_value(json_object_get(value, fields[i])) );
                        if (i != (fieldCount - 1))
                            printf("%c", delimiter);
                    } else {
                        if (i != (fieldCount - 1))
                            printf("%c", delimiter); // If NULL value is returned or Field not found then "" is returned
                    }
                }
                printf(" \n"); // New line
            }
        }
    }
}

char *returnJSONObjectAtIndex(size_t index, char *rawJSON)
{
    if (rawJSON) {
        json_t *root; // Contains all of the json data once processed by jansson
        json_error_t error; // Used as a passback for error data during json processing
        
        root = json_loads(rawJSON, 0, &error);
        if (root) {
            json_t *json_object = json_array_get(root, index);
            char *json_text = json_dumps(json_object, JSON_ENSURE_ASCII);
            json_decref(root);
            if (json_text)
                return json_text;
        }
    }
    return NULL;
}

size_t returnJSONArraySize(char *rawJSON)
{
    if (rawJSON) {
        json_t *root; // Contains all of the json data once processed by jansson
        json_error_t error; // Used as a passback for error data during json processing
        
        root = json_loads(rawJSON, 0, &error);
        if (root) {
            //size_t count = json_integer_value(json_object_get(root, "count"));
            size_t count = json_array_size(root);
            json_decref(root);
            return count;
        }
    }
    return 0;
}

const char *returnStringfromJSON(char *rawJSON, char *field)
{
    json_t *root; // Contains all of the json data once processed by jansson
    json_error_t error; // Used as a passback for error data during json processing
    
    root = json_loads(rawJSON, 0, &error);

    if (json_is_string(json_object_get(root, field))) {
        return json_string_value(json_object_get(root, field));
    }
    return NULL;
}

json_t *jsonFromObjects(json_t *json, int count,  ...)
{
    int i;
    char *objectName = NULL;
    va_list vl;
    va_start(vl,count);
    for (i=0;i<count;i++)
    {
        objectName=va_arg(vl,char *);
        json = json_object_get(json, objectName);
        if (!json) {
            printf("Error finding Object %s\n", objectName);
        }
    }
    va_end(vl);
    return json;
}

 /* This will iterate through all of ther Server Hardware in a OneView platform
  * and attempt to match the hardwareURI string to a hardwareURI that OneView is
  * aware of. If found it will then look at the hardware instance to determine if a
  * profile is assigned, if found it will return the URI of the profile
  */

char *serverProfileFromHardwareURI(oneviewSession *session, const char *hardwareURI)
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
                        const char *uri = json_string_value(json_object_get(memberValue, "uri"));
                        if (stringMatch((char *)uri, (char *)hardwareURI)) {
                            const char *serverProfile = json_string_value(json_object_get(memberValue, "serverProfileUri"));
                            setStatePath((char *)json_string_value(json_object_get(memberValue, "description")));
                            if (serverProfile) {
                                // Found the Server Profile URI
                                char *returnedProfileURI = strdup(serverProfile);
                                json_decref(hardwareJSON);
                                return returnedProfileURI;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}


/* This will iterate through all of ther Server Hardware in a OneView platform
 * and attempt to match the hardwareURI string to a hardwareURI that OneView is
 * aware of. If found it will then look at the hardware instance to determine the current
 * state of the hardware, such as if a profile is being applied.
 */

char *stateFromHardwareURI(oneviewSession *session, const char *hardwareURI)
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
                        const char *uri = json_string_value(json_object_get(memberValue, "uri"));
                        if (stringMatch((char *)uri, (char *)hardwareURI)) {
                            const char *serverHardwareState = json_string_value(json_object_get(memberValue, "state"));
                            if (serverHardwareState) {
                                // Found the Server Profile URI
                                char *returnedState = strdup(serverHardwareState);
                                json_decref(hardwareJSON);
                                return returnedState;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

/* This will iterate through all of ther Server Hardware in a OneView platform
 * and attempt to match the hardwareURI string to a hardwareURI that OneView is
 * aware of. If found it will then look at the hardware instance to determine if a
 * profile is assigned, if found it will return the URI of the profile
 */

char *ovServerPoweredOn(oneviewSession *session, char *hardwareURI)
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
                        const char *uri = json_string_value(json_object_get(memberValue, "uri"));
                        if (stringMatch((char *)uri, (char *)hardwareURI)) {
                            const char *serverProfile = json_string_value(json_object_get(memberValue, "serverProfileUri"));
                            //printf ("\n %s \n", json_dumps(memberValue, JSON_INDENT(3)));
                            if (serverProfile) {
                                // Found the Server Profile URI
                                char *returnedProfileURI = strdup(serverProfile);
                                json_decref(hardwareJSON);
                                return returnedProfileURI;
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

int mapFlexLomTosubPort(const char *explicitPort)
{
    if (stringMatch((char *) explicitPort, "a")) {
        return 1;
    } else if (stringMatch((char *) explicitPort, "b")) {
        return 2;
    } else if (stringMatch((char *) explicitPort, "c")) {
        return 3;
    } else if (stringMatch((char *) explicitPort, "d")) {
        return 4;
    }
    printf("Error matching port %s to a subport range a-c\n", explicitPort);
    return 0;
}


#endif
