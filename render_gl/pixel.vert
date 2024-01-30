#version 430

layout(location = 0) in vec3 vtx;

layout(location = 0) out vec2 oVtx;

const vec2 pos[4] = vec2[4](vec2(-1.0, 1.0),
                            vec2(-1.0, -1.0),
                            vec2(1.0, 1.0),
                            vec2(1.0, -1.0));

void main() {
    oVtx = 0.5*pos[gl_VertexID] + vec2(0.5);
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
