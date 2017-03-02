
// oneview.h

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */


#ifndef oneview_h
#define oneview_h

#define JSON_H

#define OVPERIOD_1MIN 0
#define OVPERIOD_5MIN 1

//type Definitions

typedef struct ovDebug oneviewDebug;
typedef struct ovSession oneviewSession;
typedef struct ovQuery oneviewQuery;


struct ovQuery
{
    int start;
    int count; // Number of responses from the query
    char *filter; // Filter used
    char *query; // Query
};

struct ovDebug
{
    char *usedAddress; // contains the last used address
    char *buffer; // contains the raw HTTP response
};

struct ovSession
{
    long long version;
    char *address;
    char *cookie;
    char *username;
    char *password;
    oneviewDebug *debug;
};


/*
 * initSession() - This initialise the session
 */

oneviewSession *initSession();

/*
 * loadSession(oneviewSession) - This will load the session from the home director
 */

int loadSession(oneviewSession *session);

 /* int create/delete profiles uses a profileURI
  *
  */

int ovPostProfile(oneviewSession *session, char *profile);
int ovDeleteProfile(oneviewSession *session, char *profile);

/*
 * oneviewQuery *initQuery()
 */

oneviewQuery *initQuery();

/*
 * char *ovQueryServerProfiles(oneviewSession, query string)
 */


char *ovQueryServerProfiles(oneviewSession *session, oneviewQuery *query);

char *ovQueryServerProfileTemplates(oneviewSession *session, oneviewQuery *query);

char *ovQueryNewServerProfileTemplates(oneviewSession *session, oneviewQuery *query, char *templateURI);


char *ovQueryServerHardware(oneviewSession *session, oneviewQuery *query);

char *ovQueryEnclosureGroups(oneviewSession *session, oneviewQuery *query);


/*
 * Network queries
 */

char *ovQueryNetworks(oneviewSession *session, oneviewQuery *query);


char *ovQueryNetworkSets(oneviewSession *session, oneviewQuery *query);


char *ovQueryInterconnects(oneviewSession *session, oneviewQuery *query);


char *ovQueryInterconnectGroups(oneviewSession *session, oneviewQuery *query);

char *ovQueryInterconnectStatistics(oneviewSession *session, oneviewQuery *query, char *interconnectName);

char *ovQueryInterconnectStatisticsWithURI(oneviewSession *session, oneviewQuery *query, char *uri);

char *ovQueryInterconnectStatisticsOfPort(oneviewSession *session, oneviewQuery *query, char *interconnectName, char *portName);

long long returnKBTXforInterface(oneviewSession *session, char *interface, char *rawJSON);


 /*
  * Utility Functions
  */

// Write Session ID strings to Home Directory
int writeDataToUserFile(oneviewSession *session);


//Read the Session IDs stored in the Home Directory
char* readSessionIDforHost(const char *host);
char* readSessionID(void); // return the session ID as a string from ~/.sessionID (returns NULL in error)
int readDataFromUserFile(oneviewSession *session);

// Login function
int ovLogin(oneviewSession *session);

// URL Generator for restAPI paths..
void createURL(oneviewSession *session, char *uri);
void createURLWithQuery(oneviewSession *session, oneviewQuery *query, char *uri);



int stringMatch(const char *string1, const char *string2);
// Helper function to automate the headers
void setOVHeaders(oneviewSession *session);

//Determine version of HPE OneView
long long identifyOneview(oneviewSession *session);

char *serverProfileFromHardwareURI(oneviewSession *session, const char *hardwareURI);
char *stateFromHardwareURI(oneviewSession *session, const char *hardwareURI);



#endif /* oneview_h */
