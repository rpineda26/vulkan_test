#version 450
vec2 posititions[3] = vec2[3](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
void main(){
    gl_Position = vec4(posititions[gl_VertexIndex], 0.0, 1.0);
}