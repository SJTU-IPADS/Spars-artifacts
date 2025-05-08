#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 uFragColor;

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
//   uFragColor = vec4(texture(texSampler, fragTexCoord).rgb, 0.1); // alpha值用于debug
   uFragColor = texture(texSampler, fragTexCoord);
}
