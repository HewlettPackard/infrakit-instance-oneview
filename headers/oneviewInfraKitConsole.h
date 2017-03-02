
// oneviewInfraKitConsole.h

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define LOGCRITICAL     0
#define LOGERROR        1
#define LOGWARNING      2
#define LOGNOTICE       3
#define LOGINFO         4 // Default
#define LOGDEBUG        5

int setConsolOutputLevel(unsigned int level);
int getConsoleOutputLevel();

int setStartTime();
signed long getPluginTime();

int ovPrintCritical(signed long pluginTime, const char *message);
int ovPrintError(signed long pluginTime, const char *message);
int ovPrintWarning(signed long pluginTime, const char *message);
int ovPrintNotice(signed long pluginTime, const char *message);
int ovPrintInfo(signed long pluginTime, const char *message);
int ovPrintDebug(signed long pluginTime, const char *message);
