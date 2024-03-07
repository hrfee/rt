#version 430

// The four corners of the quad (not necessarily as big as the screen),
layout(location = 64) uniform vec2 dynPos[4];

layout(location = 0) out vec2 oVtx;

// The four corners of the texture.
const vec2 pos[4] = vec2[4](vec2(-1.0, 1.0),
                            vec2(-1.0, -1.0),
                            vec2(1.0, 1.0),
                            vec2(1.0, -1.0));

void main() {
    oVtx = 0.5*pos[gl_VertexID] + vec2(0.5);
    gl_Position = vec4(dynPos[gl_VertexID], 0.0, 1.0);
}
