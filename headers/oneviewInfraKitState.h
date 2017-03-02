
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

json_t *openInstanceState();
int saveInstanceState(char *jsonData);
int appendInstanceToState(profile *foundServer, oneviewSession *session, json_t *paramsJSON);
int compareInstanceValueToKey(char *key, const char *value);
int removeInstanceFromState(char *instanceID);
int loginFromState();
char *returnValueFromInstanceKey(char *InstanceID, char *key);
int setStatePath(char *path);
char *getStatePath();
char *getArgStatePath();
int setArgStatePath(char *path);
