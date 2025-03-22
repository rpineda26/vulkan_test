#version 450
//vertex input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in ivec4 joints;
layout(location = 6) in vec4 weights;

//outputs to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out vec3 fragTangentPos;
layout(location = 5) out vec3 fragTangentView;
layout(location = 6) out vec3 fragTangentLightPos[10];

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

layout(set = 1, binding = 0) uniform sampler2D textureSampler[3]; 
layout(set = 1, binding = 1) uniform sampler2D normalSampler[3];
layout(set = 1, binding = 2) uniform sampler2D specularSampler[3];

layout(set = 2, binding = 0) uniform JointMatrixBufferObject {
    mat4 jointMatrices[100];
} jmbo;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint textureIndex;
    uint normalIndex;
    uint specularIndex;
    float smoothness;
    vec3 baseColor;
} push;

void main(){
    mat4 skinMatrix = mat4(0.0f);
    vec4 skinnedPosition = vec4(0.0f);
    
    // Check if we need to apply skinning
    bool applySkinning = true;
    // for(int i = 0; i < 4; i++) {
    //     if((weights[i] != 0) && (joints[i] > 0) && (joints[i] < 100)) {
    //         // if(joints[i]!=0){
    //         applySkinning = true;
    //         break;
    //         // }
    //     }
    // }
    
    if(applySkinning) {
        // Blend the joint matrices weighted by vertex weights
        for(int i = 0; i < 4; i++) {
            if(weights[i] == 0)
                continue;
            if(joints[i] >100){
                skinMatrix = mat4(1.0f);
                skinnedPosition = vec4(position, 1.0f);
                break;
            }
            vec4 localPosition = jmbo.jointMatrices[joints[i]] * vec4(position, 1.0f);
            skinnedPosition += localPosition * weights[i];
            skinMatrix += jmbo.jointMatrices[joints[i]] * weights[i];
        }
    } else {
        // No skinning needed, use identity matrix
        skinMatrix = mat4(1.0f);
        skinnedPosition = vec4(position, 1.0f);
    }
        
    vec4 positionWorld = push.modelMatrix * skinnedPosition;
    // vec4 positionWorld = push.modelMatrix * vec4(position, 1.0f); 
    gl_Position = ubo.projectionMatrix * (ubo.viewMatrix * positionWorld);
    fragPosition = positionWorld.xyz;
    fragColor = color * push.baseColor;
    fragUV = uv;

    //compute TBN matrix
    mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)* mat3(skinMatrix)));
    // mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal); 
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));
    //convert vectors from world space to tangent space
    vec3 cameraPosWorld = vec3(ubo.invViewMatrix * vec4(0.0, 0.0, 0.0, 1.0));
    fragNormal = N;
    fragTangentPos = TBN * fragPosition;
    fragTangentView = TBN * cameraPosWorld;
    for(int i = 0; i < ubo.pointLightCount; i++){
        PointLight pointLight = ubo.pointLights[i];
        fragTangentLightPos[i] = TBN * pointLight.position.xyz;
    }
}