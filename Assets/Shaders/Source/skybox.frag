#version 130

uniform sampler2D background;

in vec3 vertexPosition;
in vec3 iNearPosition;
in vec3 iFarPosition;

out vec4 color;

void main() {
	vec3 dir = normalize(iFarPosition - iNearPosition);

	float lng = acos(dir.y) / 3.1415;
	float lat = atan(dir.x, -dir.z) / 3.1415 / 2.0;

	color = texture(background, vec2(lat, lng));
}