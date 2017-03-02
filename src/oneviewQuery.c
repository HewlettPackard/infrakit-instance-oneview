
// oneviewQuery.c

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
#include "oneviewHttp.h"

#include <stdio.h>
#include <string.h>

char *oneViewQuery(oneviewSession *session, oneviewQuery *query, char *queryType)
{
    
    // Check that session has been initialised, an address has been set and auth cookie exists
    if (session && session->address && session->cookie) {
        
        // Wipe the contents of the allocated memory
        memset(session->debug->usedAddress, 0, 1024);
        
        // Create the url and store it in the debug structure
        createURLWithQuery(session, query, queryType);
        
        // Add the JSON test to be posted
        char *httpData;
        
        // pass the session struct for the X-API_Version
        setOVHeaders(session);
        
        // Sett HTTP Method to POST
        SetHttpMethod(DCHTTPGET);
        
        // Call the function
        httpData = httpFunction(session->debug->usedAddress);
        
        // If response exists, then return it
        if(httpData)
            return httpData;
    }
    
    return NULL; // Return nothing
}

/*
 * Server Infrastructure queries
 */

char *ovQueryServerProfiles(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/server-profiles");
}

char *ovQueryServerProfileTemplates(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/server-profile-templates");
}

char *ovQueryNewServerProfileTemplates(oneviewSession *session, oneviewQuery *query, char *templateURI)
{
    char newProfile[1024];
    snprintf(newProfile, 1024, "/%s/new-profile", templateURI);
    return oneViewQuery(session, query, newProfile);
}

char *ovQueryServerHardware(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/server-hardware");
}

char *ovQueryEnclosureGroups(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/enclosure-groups");
}

 /*
  * Network queries
  */

char *ovQueryNetworks(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/ethernet-networks");
}

char *ovQueryNetworkSets(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/network-sets");
}

char *ovQueryInterconnects(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/interconnects");
}

char *ovQueryInterconnectStatistics(oneviewSession *session, oneviewQuery *query, char *interconnectName)
{
    char statisticsAddress[1024];
    snprintf(statisticsAddress, 1024, "/rest/interconnects/%s/statistics", interconnectName);
    return oneViewQuery(session, query, statisticsAddress);
}

char *ovQueryInterconnectStatisticsWithURI(oneviewSession *session, oneviewQuery *query, char *uri)
{
    char statisticsAddress[1024];
    snprintf(statisticsAddress, 1024, "%s/statistics", uri);
    return oneViewQuery(session, query, statisticsAddress);
}


char *ovQueryInterconnectStatisticsOfPort(oneviewSession *session, oneviewQuery *query, char *interconnectName, char *portName)
{
    char statisticsAddress[1024];
    snprintf(statisticsAddress, 1024, "/rest/interconnects/%s/statistics/%s", interconnectName, portName);
    return oneViewQuery(session, query, statisticsAddress);
}

char *ovQueryInterconnectGroups(oneviewSession *session, oneviewQuery *query)
{
    return oneViewQuery(session, query, "/rest/logical-interconnect-groups");
}

/*
int ovShow(char *sessionID, int argumentCount, char *argument[])
{
    char urlString[256];
    
    if (argumentCount >=4) {
        // Check Arguments before continuing
        if (!sessionID) {
            return 1;
        }
    } else {
        return 1;
    }
    
    char *oneViewAddress = argument[1]; // IP Address of HP OneView
    char *showType = argument[3]; // Type of information to show
    char *queryType = argument[4]; // Type of Query to show
    char *httpData; // Contains all fo the data returned from a http request
//    json_t *root; // Contains all of the json data once processed by jansson
//    json_error_t error; // Used as a passback for error data during json processing
    
    // Determing the Show Type and construct the correct URL
    
    if (strstr(showType, "SERVER-PROFILES")) {
        createURL(urlString, oneViewAddress, "server-profiles");
    } else if (strstr(showType, "SERVER-HARDWARE-TYPES")) {
        createURL(urlString, oneViewAddress, "server-hardware-types");
    } else if (strstr(showType, "SERVER-HARDWARE")) {
        createURL(urlString, oneViewAddress, "server-hardware");
    } else if (strstr(showType, "ENCLOSURE-GROUPS")) {
        createURL(urlString, oneViewAddress, "enclosure-groups");
    } else if (strstr(showType, "NETWORKS")) {
        createURL(urlString, oneViewAddress, "ethernet-networks");
    } else if (strstr(showType, "NETWORK-SETS")) {
        createURL(urlString, oneViewAddress, "network-sets");
    } else if (strstr(showType, "INTERCONNECT-GROUPS")) {
        createURL(urlString, oneViewAddress, "logical-interconnect-groups");
    } else if (stringMatch(showType, "INTERCONNECTS")) {
        createURL(urlString, oneViewAddress, "interconnects");
    } else if (stringMatch(showType, "INTERCONNECTS-STATS")) {
        createURL(urlString, oneViewAddress, "interconnects/0250bd0f-4936-4099-bc95-379ddc4ea58a/statistics/X1");
    } else {
        // Display the help (to be cleared at a later date
        printf("\n SHOW COMMANDS\n------------\n");
        printf(" SERVER-PROFILES - List server profiles\n");
        printf(" SERVER-HARDWARE - list detected physical hardware\n");
        printf(" SERVER-HARDWARE-TYPES - List discovered hardware types\n");
        printf(" ENCLOSURE-GROUPS - List defined enclosure groups\n");
        printf(" NETWORKS - List defined networks\n");
        printf(" INTERCONNECTS - List uplinks\n\n");
        return 1;
    }
    
    // If a query has been submitted speak to OneView and process the results
    if (queryType) {
        
        // Pass the URL and sessionID to HP OneView and return the response
        appendHttpHeader("Content-Type: application/json");
        appendHttpHeader(OV_Version);
        SetHttpMethod(DCHTTPGET);
        createHeader("Auth: ", sessionID);
        httpData = httpFunction(urlString);
        
        // If response is empty fail
        if(!httpData)
            return 1;
        
        // Process the raw http data and free the allocated memory
        root = json_loads(httpData, 0, &error);
        free(httpData);
        
        
        
        if (strstr(queryType, "RAW")) {
            char *json_text = json_dumps(root, JSON_ENSURE_ASCII);
            printf("%s\n", json_text);
            free(json_text);
            return 0;
        } else if (strstr(queryType, "PRETTY")) {
            char *json_text = json_dumps(root, JSON_INDENT(4)); //4 is close to a tab
            printf("%s\n", json_text);
            free(json_text);
            return 0;
        } else if (strstr(queryType, "URI")) {
            // will return URI and Name
            json_t *memberArray = json_object_get(root, "members");
            if (json_array_size(memberArray) != 0) {
                size_t index;
                json_t *value;
                json_array_foreach(memberArray, index, value) {
                    const char *uri = json_string_value(json_object_get(value, "uri"));
                    const char *name = json_string_value(json_object_get(value, "name"));
                    printf("%s \t %s\n", uri, name);
                }
            }
            return 0;
        } else if (strstr(queryType, "FIELDS")) {
            
            //int fieldCount = argc -5; //argv[0] is the path to the program
            json_t *memberArray = json_object_get(root, "members");
            if (json_array_size(memberArray) != 0) {
                size_t index;
                json_t *value;
                json_array_foreach(memberArray, index, value) {
                    for (int i = 5; i< argumentCount; i++) {
                        if (json_is_number(json_object_get(value, argument[i]))) {
                            printf("%lli \t", json_integer_value(json_object_get(value, argument[i])));
                        } else if (json_is_string(json_object_get(value, argument[i]))) {
                            printf("%s \t", json_string_value(json_object_get(value, argument[i])));
                        } else {
                            printf("\"\" \t"); // If NULL value is returned or Field not found then "" is returned
                        };
                    }
                    printf(" \n"); // New line
                }
            }
            return 0;
            
        }
    }
    
    // Either a unknown query or no query was submitted so display help
    printf("\n\nPlease enter output:\n");
    printf("RAW: ASCII ENCODED RAW JSON\n");
    printf("PRETTY: Parsed and Indented JSON\n");
    printf("URI: Lists NAME and URI\n");
    printf("\n\n");
    
    // Should probably re-address the return value at a later time
    return 0;
}

void ovShowPrintHelp()
{
    printf("\n OVCLI xxx.xxx.xxx.xxx SHOW <TYPE> <OUTPUT>");
    printf("\n <OUTPUT>");
    printf("\n\t RAW - raw JSON");
    printf("\n\t PRETTY - readable JSON");
    printf("\n\t URI - prints NAME and URI fields");
    printf("\n\t FIELDS <FIELD 1> <FIELD 2> <FIELD 3>");
    printf("\n <TYPE>");
    printf("\n\t NETWORKS <OUTPUT>");
    printf("\n\t SERVER-PROFILES <OUTPUT>");
}

*/
