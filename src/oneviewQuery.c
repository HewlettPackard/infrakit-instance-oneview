
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

