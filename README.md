# InfraKit-Instance-OneView
This is an InfraKit Instance Plugin that is designed to automate the provisioning of OneView Instances (Servers currently) through InfraKit from Docker.


## Architecture Overview

The Instance plugin will take the configuration that is described in the Group JSON configuration and commit the instance code to the plugin itself. The plugin then will communicate directly with OneView to assess the state of defined instances and act accordingly (**create**, **grow**, **heal**, **remove** or **destroy**) the instances.

![OneView Architecture](http://thebsdbox.co.uk/wp-content/uploads/2016/11/InfraKit-Instance-oneview.jpeg)

## Notes

### State data

One other area that needs to be considered is the `int synchroniseStateWithProvider()` function, as this function provides the capability to speak to your instance provider and determine if the plugin state matches. It will iterate through the state data and then compare each instance to the state from the provider. If also is designed around physical devices so it's possible to move a device from the active instances to the non-functional instance array and vice-versa. When an instance is non-functional it wont be reported back to InfraKit, therefore a new instance will be provisioned. When an instance is moved from non-functional to active instances, it will be added to the front of the array meaning that the last added instance will be destroyed.

## To build:

* Grab the source, either download the zip or git pull
* run the `./build_libs.sh` to download the jansson JSON library and build it for your Architecture
* run `make`

You'll be left with a infrakit-instance-c that will start your plugin, for further help run `./infrakit-instance-c --help`


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

#### ToDo


However there is a small memory leak in the HTTPD server (simple fix). 