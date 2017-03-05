
// oneviewHTTP.h

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */

#ifndef dcHttp_h
#define dcHttp_h

#include <stdio.h>

#endif /* dcHttp_h */

void setHttpAuth(char* authString);
void appendHttpHeader(char *header);
void setHttpData(const char* dataString);
void setHttpPort (long httpPort);
void SetHttpMethod(int method);
char *httpFunction(char *url);
void PrintHttpAuth();
void createHeader(char *key, const char *data);

#define DCHTTPPOST     0 // POST Operation
#define DCHTTPPUT      1 // PUT Operation
#define DCHTTPGET      2 // GET Operation
#define DCHTTPDELETE   3 // DELETE Operation5
