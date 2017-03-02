
 // oneviewInfraKitInstance.h
 
/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */

#include "jansson.h"

#ifndef PROFILE_H
#define PROFILE_H

#define INSTANCE_SERVER_PROFILE 0
#define INSTANCE_NETWORK 1
#define INSTANCE_STORAGE 2

typedef struct {
    char *instanceName;          // pointer to name of instance
    size_t instanceType;              // signed integer to hold the instance type
} instance;

typedef struct {
    char *profileName;          // pointer to name of Profile
    char *templateName;         // pointer to name of Template Profile
    char *uri;                  // pointer to uri of Profile
    char *availableHardwareURI; // pointer to uri of available Hardware
    char *hardwareTypeUri;      // pointer to hardwareURI string of profile
    char *enclosureUri;         // pointer to hardwareURI string of profile
} profile;

typedef struct {
    char *networkName;          // pointer to name of network
    size_t vlanID;              // signed integer to hold the vlanID
} network;

#endif

instance *processInstanceJSON(json_t *json_text, long long id);

char *ovInfraKitInstanceDescribe(json_t *params, long long id);
char *ovInfraKitInstanceProvision(json_t *params, long long id);
char *ovInfraKitInstanceDestroy(json_t *params, long long id);

int instanceLogin(const char *address, const char *username, const char *password);

// Functions to tidy and return memory back to the heap

int freeServerProfile(profile *freeProfile);
int freeNetwork(network *freeNetworkInstance);
int freeInstance(instance *freeInstance);
