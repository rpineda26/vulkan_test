#version 450

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;

} ubo;

void main() {
    float dis = sqrt(dot(fragOffset, fragOffset));
    if(dis >= 1.0) {
        discard;
    }
    outColor = vec4(ubo.lightColor.xyz, 1.0);
}