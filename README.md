simple vulkan graphics engine\

dependencies: Vulkan, GLFW, GLM\
Note: The path for the VulkanSDK, GLFW, and GLM must be provided in the .env.cmake though in some devices these packages can be found by the Cmake script without specifying the path.\
provided dependencies: tinyobj, stb_image, dear imgui\
These dependencies are already provided in \external so their path in the .env are optional

For Windows:\
If using mingw: provide the mingw path in the .env.cmake\
If using visual studio, specify the proper version in the cmakelist.txt line 41: `set(GLFW_LIB "${GLFW_PATH}/lib-vc2019")`

to build and exectute the program:\
<mark>unix systems</mark>\
`chmod +x unixBuild.sh`\
`./unixhBuild.sh`\
this script includes running the executable file

<mark>windows</mark>\
`./mingwBuild.bat`\
the executable file can be found at 'build/VulkanEngine.exe'


Controls: Select the object in the imGui editor first\
keyboard input:\
//macros to select objects to move\

//movement\
wasd - walk\
arrow keys - rotate\

Features:\
select object to change property:\
point light: change color, intensity, radius\
game objects: change model, base color, albedo map, normal map, specular map, smoothness\
add objects: point lights (Maximum of 10) and game objects\
Toggle outline highlight: (works well on well defined edges).

Here are the online references for this program\
general project structure and setup:\
https://www.youtube.com/watch?v=Y9U9IE0gVHA&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR&index=1\
https://vulkan-tutorial.com/Depth_buffering\
normal mapping implementation\
https://learnopengl.com/Advanced-Lighting/Normal-Mapping\
user input\
https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/
