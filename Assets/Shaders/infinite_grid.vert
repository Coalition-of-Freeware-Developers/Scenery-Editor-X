#version 330 core

out vec3 WorldPos;
uniform mat4 gVP;
uniform vec3 gCameraWorldPos;

const vec3 Pos[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0), // Bottom Left
    vec3(1.0, 0.0, -1.0), // Bottom Right
    vec3(1.0, 0.0, 1.0), // Top Right
    vec3(-1.0, 0.0, 1.0), // Top Left
);

const int Indicies[6] = int[6] (0, 2, 1, 2, 0, 3);

void main() {
    int Index = Indicies[gl_VertexID];
    vec3 vPos = Pos[Index];

    vPos3.x += gCameraWorldPos.x;
    vPos3.z += gCameraWorldPos.z;

    vec4 vPos = vec4(Pos[Index], 1.0);
    gl_Position = gVP * vPos;

    WorldPos = vPos3;
}