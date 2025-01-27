simple vulkan graphics engine
UPDATE: Press space bar to change textures

dependencies: Vulkan, GLFW, GLM
provided dependencies: tinyobj, stb_image

to build and exectute the program:
==unix systems==
`chmod +x unixBuild.sh`
`./unixhBuild.sh`
this script includes running the executable file

==windows==
`./mingwBuild.bat`
the executable file can be found at 'build/VulkanEngine.exe'


Controls:
keyboard input:
//macros to select objects to move
1 = select camera
2 = select light source
3 = select vase
4 = select cube
5 = select floor

//movement
wasd - walk
arrow keys - rotate
mouse nive - rotate



Here are the online references for this program
general project structure and setup:
https://www.youtube.com/watch?v=Y9U9IE0gVHA&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR&index=1
https://vulkan-tutorial.com/Depth_buffering
normal mapping implementation
https://learnopengl.com/Advanced-Lighting/Normal-Mapping
user input
https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/
