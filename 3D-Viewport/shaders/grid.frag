#version 130

out vec4 color;

in vec3 vertexPosition;
in vec3 near;
in vec3 far;

uniform mat4 iProjection;
uniform mat4 iModelView;

float checkerboard(vec2 R, float scale) {
	return float((
		int(floor(R.x / scale)) +
		int(floor(R.y / scale))
	) % 2);
}

float computeDepth(vec3 pos) {
	vec4 clip_space_pos = iProjection * iModelView * vec4(pos.xyz, 1.0);
	float clip_space_depth = clip_space_pos.z / clip_space_pos.w;

	float far = gl_DepthRange.far;
	float near = gl_DepthRange.near;

	float depth = (((far-near) * clip_space_depth) + near + far) / 2.0;

	return depth;
}

void main() {
	float t = -near.y / (far.y-near.y);

	vec3 R = near + t * (far-near);

	float c =
		checkerboard(R.xz, 1) * 0.3 +
		checkerboard(R.xz, 10) * 0.2 +
		checkerboard(R.xz, 100) * 0.1 +
		0.1;
	c = c * float(t > 0);

	float spotlight = min(1.0, 1.5 - 0.02*length(R.xz));

	color = vec4(vec3(c*spotlight), 1);

	gl_FragDepth = computeDepth(R);
}