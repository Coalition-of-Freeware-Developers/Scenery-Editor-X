#version 330 core
layout (location = 1) in vec3 aPosition;
out vec2 fragCoord;

uniform mat4 transform;

void main()
{
	fragCoord = aPosition.xy;
	gl_Position = transform * vec4(aPosition, 1.0);
}