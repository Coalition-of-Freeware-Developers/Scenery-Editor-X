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

* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#windows)
* [Boost C++ Library v1.86](https://www.boost.org/users/history/version_1_86_0.html)
[NOTE] I have pondered with the idea of just building the boost library components into their respective `.dll` files and embeding them into the Scenery Editor X project to reduce the amount of dependencies needed.
This has not been done at the time of me updating this read-me but I will do so when I cleanup this code.

## Environment Variables 
To be able to build this program you must have a few environment variables set in your system to be able to be referenced in the code base.
Environment Variables:
### User Variables
* BOOST_INCLUDE - Path to the Boost C++ Librarys' folder.
* BOOST_LIB - Path to the Boost C++ Library `stage\lib` folder.
* XPLANE_12_SDK - Path to where your XPlane 12 SDK folder is.

### System Variables
* Add this to your "Path" veriable list: `C:\VulkanSDK\1.3.296.0\Bin`
* VULKAN_SDK - `C:\VulkanSDK\1.3.296.0\Bin`

[NOTE] Check to make sure this variable is set. Vulkan's installer will set this when installed but make sure it is there or else vulkan won't build.

## Build

From command line to download this repository and all its submodules use command: 
`git clone --recurse-submodules https://github.com/The3dVehicleguy/Scenery-Editor-X.git`



### CMake:
Cmake build is a work on progress.

