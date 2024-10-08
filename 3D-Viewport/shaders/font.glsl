#version 120

#define SRGB 1

// Set this to 1 to view the raw font texture to view alignment.
#define DEBUG 0

#if SRGB
vec3 to_linear(vec3 srgb)
{
	return mix(
			pow(srgb * (1.0/1.055f) + (0.055f/1.055),vec3(2.4)),
			srgb * (1.0 / 12.92), 			
			vec3(lessThanEqual(srgb,vec3(0.04045))));
}
#endif

#if VSHADER
	attribute vec4 a_vertex;
	attribute vec4 a_color;
	attribute vec2 a_texcoord0;

	#if BORDER
		attribute vec4 a_texcoord1;
	#endif
#endif

	varying vec4 v_color;
	varying vec2 v_texcoord0;
	
	#if BORDER	
		varying vec4 v_texcoord1;
	#endif

	uniform mat4 mvp_matrix;
	uniform vec3 mv_offset;
	uniform	sampler2D u_tex;

#if VSHADER

void main()
{
	gl_Position = mvp_matrix * (a_vertex - vec4(mv_offset,0.0));	
	v_texcoord0 = a_texcoord0;

	#if SRGB
		v_color = vec4(to_linear(a_color.rgb),a_color.a);
	#else
		v_color = a_color;
	#endif
	#if BORDER
		#if SRGB
			v_texcoord1 = vec4(to_linear(a_texcoord1.rgb),a_texcoord1.a);
		#else
			v_texcoord1 = a_texcoord1;
		#endif
	#endif
}

#endif

#if FSHADER

void main()
{
	#if BORDER
		vec4	texel = texture2D(u_tex, v_texcoord0);		
		
		#if DEBUG
			gl_FragColor.rg = texel.rg;
			gl_FragColor.ba = vec2(0.0,1.0);
		
		#else
			float alpha_outline = texel.g * v_texcoord1.a;
			float alpha_main = texel.r * (1.0 - alpha_outline) * v_color.a;
			
			gl_FragColor.rgb = (v_color.rgb * alpha_main + v_texcoord1.rgb * alpha_outline);
			gl_FragColor.a = (alpha_outline + alpha_main);
		#endif
		
	#else
	
		float alpha = texture2D(u_tex, v_texcoord0).r * v_color.a;
		gl_FragColor = vec4(v_color.rgb, alpha);
		
	#endif
}

#endif