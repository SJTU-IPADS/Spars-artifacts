#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
   gl_Position = vec4(inPos, 0.0, 1.0);
   fragTexCoord = inTexCoord; //纹理坐标，会被插值后传递给片段着色器
}
