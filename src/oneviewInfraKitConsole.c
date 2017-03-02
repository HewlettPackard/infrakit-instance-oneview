
// oneviewInfraKitConsole.c

/* instance-infrakit-oneview - A Docker InfraKit plugin for HPE OneView
 *
 * Dan Finneran <finneran@hpe.com>
 *
 * (c) Copyright [2017] Hewlett Packard Enterprise Development LP;
 *
 * This software may be modified and distributed under the terms
 * of the Apache 2.0 license.  See the LICENSE file for details.
 */


#include "oneviewInfraKitConsole.h"
#include <stdio.h>
#include <time.h>


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int outputLevel = 4;

time_t pluginStartTime;

/* These functions will set an initial seconds since UNIX epoch
 * it will then use this inital time to provide a seconds that the plugin
 * has been executing for
 */

int setStartTime()
{
    pluginStartTime = time(NULL);
    if (pluginStartTime == -1) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

signed long getPluginTime()
{
    time_t currentPluginTime = time(NULL);
    if (currentPluginTime !=1) {
        return currentPluginTime - pluginStartTime;
    }
    return 0;
}

int setConsolOutputLevel(unsigned int level)
{
    if (level < 6) {
        outputLevel = level;
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int getConsoleOutputLevel()
{
    return outputLevel;
}

int ovPrintCritical(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGCRITICAL) {
        printf(ANSI_COLOR_MAGENTA"CRIT[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int ovPrintError(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGERROR) {
        printf(ANSI_COLOR_RED"ERRO[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int ovPrintWarning(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGWARNING) {
        printf(ANSI_COLOR_YELLOW"WARN[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int ovPrintNotice(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGNOTICE) {
        printf(ANSI_COLOR_GREEN"NOTE[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int ovPrintInfo(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGINFO) {
        printf(ANSI_COLOR_BLUE"INFO[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int ovPrintDebug(signed long pluginTime, const char *message)
{
    if (outputLevel >= LOGDEBUG) {
        printf(ANSI_COLOR_BLUE"DEBG[%05zu]"ANSI_COLOR_RESET" %s\n", pluginTime, message);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
