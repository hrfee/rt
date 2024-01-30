#version 430

layout(location = 0) in vec2 oVtx;

layout(binding = 0) uniform sampler2D imgTex;

layout(location = 0) out vec3 colour;

void main() {
    colour = texture(imgTex, oVtx).rgb;
}
