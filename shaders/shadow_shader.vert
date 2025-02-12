#version 450
#extension GL_ARB_shader_viewport_layer_array : enable

layout(location = 0) in vec3 position;


struct Light{
    mat4 lightSpaceMatrix[6];
    vec4 lightPos;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    Light light;
    int pointLightCount;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;

} push;

void main(){
    int faceIndex = gl_InstanceIndex % 6;
    gl_Position = ubo.light.lightSpaceMatrix[faceIndex] * push.modelMatrix * vec4(position, 1.0);
    gl_Layer = faceIndex;
}