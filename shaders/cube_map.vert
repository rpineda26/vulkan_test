#version 450
layout(location = 0) in vec3 position;
layout(location = 0) out vec3 fragUVW;

struct PointLight {
    vec4 position;
    vec4 color;
    float radius;
    int objId;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    PointLight pointLights[10];
    int pointLightCount;
    int selectedLight;
    float time;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main(){
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * push.modelMatrix * vec4(position*100.0, 1.0);

    fragUVW = position;
}