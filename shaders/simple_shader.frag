#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragUv;
layout(location = 0) out vec4 outColor;
//camera view plus lighting
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 invViewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;

} ubo;
layout(set = 0, binding = 1) uniform sampler2D textureSampler; 
layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;
void main() {
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormal);
    vec3 cameraPosWorld = ubo.invViewMatrix[3].xyz;
    vec3 viewDir = normalize(cameraPosWorld - fragPosition);

    vec3 directionToLight = ubo.lightPosition.xyz - fragPosition;
    float attenuation = 1.0 / dot(directionToLight, directionToLight) * 10;
    directionToLight = normalize(directionToLight);

    vec3  lightColor = ubo.lightColor.xyz* ubo.lightColor.w *  attenuation;
    vec3 ambientLightColor = ubo.ambientLightColor.xyz  * ubo.ambientLightColor.w;
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 diffuseLight = lightColor * cosAngIncidence + ambientLightColor;

    vec3 halfAngle = normalize(directionToLight + viewDir);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0.0, 1.0);
    blinnTerm = pow(blinnTerm, 128.0);
    specularLight += ubo.lightColor.xyz * attenuation * blinnTerm;

    vec3 texColor = texture(textureSampler, fragUv).rgb;
    outColor = vec4(diffuseLight * fragColor + specularLight  * texColor, 2);
}