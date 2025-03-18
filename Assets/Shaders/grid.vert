#version 330 core

uniform mat4 gVP;
uniform vec3 camPos;         // World Space Camera to get square to move with camera


const vec3 Pos[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0), // Bottom Left
    vec3( 1.0, 0.0, -1.0), // Bottom Right
    vec3( 1.0, 0.0,  1.0), // Top Right
    vec3(-1.0, 0.0,  1.0)  // Top Left
);

// 6 Vertices indices       |Triangle1|Triangle2|
const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);


void main()
{
    int Index = Indices[gl_VertexID]; //Access the vertex index

    vec3 vPos3 = Pos[Index];

    vPos3.x += camPos.x; // Offset of square by camera
    vPos3.z += camPos.z; // Always 0

    vec4 vPos = vec4(vPos3, 1.0); //Extend vec3 to vec4

    gl_Position = gVP * vPos; //Multiply vertex coord by camera position
}
