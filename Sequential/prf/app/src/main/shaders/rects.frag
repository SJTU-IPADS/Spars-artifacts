#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 uFragColor;
layout(location = 0) in vec3 fragColor;

void main() {
   uFragColor = vec4(fragColor, 1.0); // alpha值用于debug
}
