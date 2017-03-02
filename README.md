# infrakit-instance-oneView
This is a Docker InfraKit Instance Plugin that is designed to automate the provisioning of "Instances" through HPE OneView (Servers currently). This plugin is driven by configuration that is passed in to Docker InfraKit typically using the group plugin to manage numerous instances or numerous groups of instances (multi-tenancy of sorts).


## Architecture Overview

The Instance plugin will take the configuration that is described in the group-default JSON configuration and commit the instance code to the plugin itself. The plugin then will communicate directly with OneView to assess the state of defined instances and act accordingly by (**createing**, **growing**, **healing**, **removing** or **destroying**) the instances. If numerous group configurations are commited, then the oneview plugin will manage all instances and will differentiate between instances and which group they belong too.

![OneView Architecture](http://thebsdbox.co.uk/wp-content/uploads/2016/11/InfraKit-Instance-oneview.jpeg)

### Plugin socket
Ideally the various Docker InfraKit plugins are meant to be started inside of containers, to expose communication between the various plugins (which takes place over UNIX sockets) the path where the sockets are created should be mapped with the `-v` docker flag. Like the Docker InfraKit standard all plugin sockets are created in `$HOME/.infrakit/plugins`.

### State data

In order to manage expected state with actual state, there are two methods that are used to keep state regulated:

* State JSON
* Group tags

The **State JSON** is created by the plugin and lives in `$HOME/.infrakit/state/`. As instances are created through the instance plugin, they are also added to the state JSON. This then allows the plugin the functionality to compare the expected state (state JSON) to the state of the physical Infrastructure (HPE OneView). If a server has failed or the server profile has been removed, then the plugin can report back to the group plugin that an instance is missing and InfraKit will act accordingly (typically by auto-healing the Infrastructure).

The **Group Tags** live inside of HPE OneView instances are used in order to allow the plugin to determine which state file (detailed above) an instances is described within. 

Both of these two methods for state data provide a stable method for the plugins to manage instance states and react accordingly to Hardware changes or plugin restarts.

## Using the plugin

**Starting**

The plugin can be started quite simply by running `./infrakit-instance-oneview` which will start the plugin with all of the defaults for **socket** and **state** files located within the `~/.infrakit` directories. Once the plugin is up and running it can be discovered through the InfraKit cli through the command `infrakit plugin ls`. 

**Configuration**

To pass authentication credentials to the HPE OneView plugin, it should be started with a number of environment variables:

`OV_ADDRESS` = IP address of OneView

`OV_USERNAME` = Username to connect to OneView

`OV_PASSWORD` = Password to authenticate to OneView

As with all InfraKit plugins, the group plugin will define the *"amount"* of instances that need to be provisioned the instance plugins. The group plugin will then pass the instance configuration to the plugin as many times as needed. The main points of note in the instance configuration:

`TemplateName` = **[required]** This has to match (case sensitive) a pre-created OneView template

`ProfileName` = This is a prefix to created instances e.g. `{ProfileName}-12453647587698023425365`

`OneView : {} ` = **[deprecated]** This should only be used for testing and will generate warnings, use environment variables for these settings (see above)

```
"Instance": {
      "Plugin": "instance-oneview",
      "Properties": {
        "Note": "Generic OneView configuration",
        "OneView": {
          "OneViewAddress": "192.168.0.96",
          "OneViewUsername": "Administrator",
          "OneViewPassword": "password"
        },
        "TemplateName": "Docker-Gen8-Template",
        "ProfileName" : "Docker"
      }
    },
```

## To build

* Grab the source, either download the zip or git pull
* run the `./build_libs.sh` to download the jansson JSON library and build it for your Architecture
* run `make`

You'll be left with a infrakit-instance-oneview that will start your plugin, for further help run `./infrakit-instance-oneview --help`


```
$ ./infrakit-instance-oneview --help
InfraKit Instance Plugin for Docker

 Usage:
 ./infrakit-instance-oneview [flags]

 Available Commands:
 version		 print build version information

 Flags:
	--name	Plugin name to advertise
	--log	Logging level, maximum 5 being the most verbose
	--state	Path to a state file to handle instance state information
```

## NEXT STEPS


[ More to come]


# Copyright and license

Copyright © 2017 Hewlett-Packard Enterprise. Released under the Apache 2.0 license. See [LICENSE](https://github.com/thebsdbox/infrakit-instance-oneview/raw/master/LICENSE) for the full license.