
// oneviewInfraKitState.h

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
#include "oneviewInfraKitInstance.h"

#include "jansson.h"

 /*State file funcitons
  */
json_t *openInstanceState();
int saveInstanceState(char *jsonData);
int setStatePath(char *path);
char *getStatePath();
char *getArgStatePath();
int setArgStatePath(char *path);

int loginFromState(const char *groupName);

// Add remove from state
int appendInstanceToState(profile *foundServer, oneviewSession *session, json_t *paramsJSON);
int removeInstanceFromState(const char *instanceID, const char *groupName);

// search state
char *returnInstanceFromState(const char *InstanceID, char *key);
json_t *returnObjectFromInstanceID(const char *InstanceID);
json_t *findGroup(json_t *state, const char *groupName);
int findUsedHWInState(const char *hardwareURI);
