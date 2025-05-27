# Scenery Editor X 
Scenery Editor X is a editor tool for use in creating, modifying, and building custom X-Plane 12 scenery. Unlike similar editing software this program utilizes a 3D viewport to allow the user to quickly and rapidly manipulate and create sceneries with more adaptive and modern tools and utilities. 

### Features
* Interactive and responsive 3D viewport
* X-Plane Library asset management tools
* Ability to import `.USD`, `.obj`,`.obj8`, `.fbx`, and `.gltf` files and export with the project as a library item converted into the X-Plane `.obj8` format. 
* Ability to create and edit X-Plane specific formats such as; `.lin`,  `.pol`, `.fac`, `.ags`, `.agb`, and `.str` assets.
* Provide visual editor tools for asset creation and manipulation
* Replicate graphics shaders to achieve an accurate representation of the scenery when implemented. 
* Compatible with the X-Plane Scenery Gateway.
* Time of day visualization 
* Plugin system to allow custom tools and features to be implemented to meet the users need. 
* Ability to open and convert WED projects into Scenery Editor X projects.

## Dependencies
Inorder to compile and develop Scenery Editor X you will need:

* [CMake](https://cmake.org/download/)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)

## Environment Variables 
To be able to build this program you must have a few environment variables set in your system to be able to be referenced in the code base.
Environment Variables:
### User Variables
* XPLANE_12_SDK - Path to where your XPlane 12 SDK folder is.

### System Variables
* Add this to your "Path" veriable list: `C:\VulkanSDK\1.3.296.0\Bin`
* VULKAN_SDK - `C:\VulkanSDK\1.3.296.0\Bin`

[NOTE] Check to make sure this variable is set. Vulkan's installer will set this when installed but make sure it is there or else vulkan won't build.

## Build

This repository uses SSH keys for access. It helps to have the key as pulling the repository will also pull and update the submodules.
From command line to download this repository and all its submodules use command: 

HTTP: `git clone --recurse-submodules https://github.com/The3dVehicleguy/Scenery-Editor-X.git`
or
SSH: `git clone --recurse-submodules git@github.com:The3dVehicleguy/Scenery-Editor-X.git`

### CMake:
Please make sure you have CMake installed and added to your environment variable path.
I tried to make this as easy and simple as possible to get you up and running. All you need to do is run `Setup.bat` and follow the prompts on the command prompt window.
After that you will have the whole project setup and located in the `build/` folder.

[NOTE] Use the `Setup.bat` script if you need to regenerate the project structure.
