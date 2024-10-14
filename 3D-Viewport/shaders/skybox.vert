#version 130

in vec3 P;
in vec3 iNearPositionVert;
in vec3 iFarPositionVert;

out vec3 vertexPosition;
out vec3 iNearPosition;
out vec3 iFarPosition;

void main() {
	// pass all parameters unchanged
	vertexPosition = P;
	iNearPosition = iNearPositionVert;
	iFarPosition = iFarPositionVert;

	// the position of each vertex in screen space
	vec4 pos4 = vec4(P.x, P.y, 1, 1);
	gl_Position = pos4;
}