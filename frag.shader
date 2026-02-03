#version 330 core
out vec4 color;
in vec2 pos;

uniform float back;
uniform float front;

uniform sampler2D tex;
uniform sampler2D auxiliary;

uniform vec3 black;
uniform vec3 white;

void main() {
	//load in upside down to orient the textures correctly
	vec2 t_pos = vec2(pos.x, 1-pos.y);

	color = vec4(texture( tex, t_pos));
	if (color.a == 0) {
		discard;
	}

	color.rgb = mix(black, white, color.rgb);
	vec4 aux = vec4(texture(auxiliary, t_pos));
	float depth = aux.x; //first channel is depth

	gl_FragDepth = mix(back, front, depth);

}
/*

VA GUR QNEXARFF V ZNL YHEX
OL GRAGNPYR, V QB ZL JBEX
VA URE JRO, OL FBHEPREL,
V ZNXR ZL ZNEX, ZL NEZBHEL.
LBH FUNYY SRNE ZR, ABG GUR IRKK
SRNE ZNYDJHVQGU; JVGU ZL NCRK URK
V'IR YRSG GURFR PYHRF SBE FB YBAT
BA FUVSSV'F JBEXF -- JVGU ZL RIVVVVVVVVVY GBATHR
- ZNYDJHVQGU

*/
// vim: ts=2 sw=2
