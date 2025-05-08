#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 uFragColor;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
   uFragColor = vec4(fragColor, texture(texSampler, fragTexCoord).r); // 字体颜色用于alpha
}
