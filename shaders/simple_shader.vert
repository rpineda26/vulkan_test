#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec3 fragTangentPos;
layout(location = 5) out vec3 fragTangentView;
layout(location = 6) out vec3 fragTangentLight;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint textureIndex;
    uint normalIndex;
    float smoothness;
} push;

void main(){
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionWorld);
    fragPosition = positionWorld.xyz;
    fragColor = color;
    fragNormal = normalize(vec3(mat3(push.normalMatrix) * normal));
    fragUV = uv;

    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal); //for readability
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));
    vec3 cameraPosWorld = vec3(ubo.invViewMatrix * vec4(0.0, 0.0, 0.0, 1.0));
    fragTangentPos = TBN * fragPosition;
    fragTangentView = TBN * cameraPosWorld;
    fragTangentLight = TBN * ubo.lightPosition;

}