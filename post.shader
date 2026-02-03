#version 330 core
out vec4 color;
in vec2 pos;
uniform sampler2D tex;
uniform int sparkle = 1;
//must be in [0,1)
uniform float random_seed = 0.4;

uniform float pixel_size_x = 0.0015;
uniform float pixel_size_y = 0.0015;

const float bloom_brightness = 0.08;
const int bloom_size = 12;

const float glow_power = 0.85;

// from https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl#answer-28095165
float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float gold_noise(in vec2 xy, in float seed){
       return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

vec4 bloom_part(vec2 t_pos, int i, int j, vec2 p) {
	vec2 sample_pos = t_pos + vec2(pixel_size_x*i*p.x, 
										pixel_size_y*i*p.y);
	vec4 raw_color = texture( tex, sample_pos);
	
	vec2 pixel_loc = sample_pos / vec2(pixel_size_x, pixel_size_y);

	float sparkle_power = gold_noise(pixel_loc, random_seed);
	sparkle_power *= sparkle_power;
	sparkle_power *= sparkle_power;
	
	if (raw_color.r + raw_color.g + raw_color.b <= 3.0) {
		return vec4(0);
	} else {
		return sparkle_power*raw_color*bloom_brightness*j;
	}
}

//straight green
const vec3 CHROMA_KEY = vec3(0,1,0); 

void main() {
	vec2 t_pos = vec2((pos.x+1)/2, (pos.y+1)/2);

	color = texture( tex, t_pos);
	vec4 glow = vec4(0,0,0,0);
	if (color.rgb == CHROMA_KEY) return;
	
	if (sparkle == 1) {
		int i;
		for (i=1; i<bloom_size; i++) {
			int j = bloom_size-i;
			
			glow += bloom_part(t_pos, i,j, vec2(1, 1));
			glow += bloom_part(t_pos, i,j, vec2(1,-1));
			glow += bloom_part(t_pos, i,j, vec2(-1, -1));
			glow += bloom_part(t_pos, i,j, vec2(-1, 1));
		}

		color += min(glow, vec4(glow_power, glow_power, glow_power, 1));
	} else if (sparkle == 2) {
		vec2 pixel_loc = t_pos / vec2(pixel_size_x, pixel_size_y);
		color = vec4(0,0,gold_noise(pixel_loc, random_seed), 1);
	} else if (sparkle == 3) {
		color = min(texture(tex, t_pos+vec2(pixel_size_x, pixel_size_x)), color);
		color = min(texture(tex, t_pos+vec2(pixel_size_x, -pixel_size_x)), color);
		color = min(texture(tex, t_pos+vec2(-pixel_size_x, pixel_size_x)), color);
		color = min(texture(tex, t_pos+vec2(-pixel_size_x, -pixel_size_x)), color);
		color = min(texture(tex, t_pos+vec2(2*pixel_size_x, 0)), color);
		color = min(texture(tex, t_pos+vec2(0,2*pixel_size_x)), color);
		color = min(texture(tex, t_pos+vec2(-2*pixel_size_x, 0)), color);
		color = min(texture(tex, t_pos+vec2(0,-2*pixel_size_x)), color);
	}
}

// vi: ts=2 sw=2 
