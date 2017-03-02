
// oneviewJSONParse.h

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

#ifndef oneviewJSONParse_h
#define oneviewJSONParse_h

#include <stdio.h>
#include "oneview.h"


// output types
#define JSON_RAW 0
#define JSON_READABLE 1

// profileStatistic
#define OV_PROFILE_STATISTIC_DIFF 0 // previous statistic - latest = difference
#define OV_PROFILE_STATISTIC_CURR 1 // latest statistic is set in the profile array


void ovParseMembers(char *rawJSON, char delimiter, char *fields[], int fieldCount);
char *returnRawJSONFromObject(char* rawJSON, char *objectName);
char *returnReadableJSONFromRaw(char *rawJSON);
size_t returnJSONArraySize(char *rawJSON);
char *returnJSONObjectAtIndex(size_t index, char *rawJSON);
void ovParseArray(char *rawJSON, char *arrayName, char delimiter, char *fields[], int fieldCount);
const char *returnStringfromJSON(char *rawJSON, char *field);


#endif /* oneviewJSONParse_h */
