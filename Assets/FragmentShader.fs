#version 330

struct SpotLight {
    vec3 position;
    vec3 rgbIntensity;
    float cosCutOff;
    vec3 dir;
    float exp;
};

in VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
	SpotLight light;
	vec2 tex;
} fs_in;


out vec4 fragColour;

struct Material {
    vec4 kd;
    vec3 ks;
    float shininess;
};
uniform Material material;

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;

uniform sampler2D ourTexture;
uniform bool textured;
uniform bool infrared;

vec3 phongModel(vec3 fragPosition, vec3 fragNormal) {
    vec3 tex;
    if (textured) {
    	tex = texture(ourTexture, fs_in.tex).xyz;
    }
    SpotLight light = fs_in.light;
    // Direction from fragment to light source.
    vec3 l = normalize(light.position - fragPosition);
    float spotdot = dot(-l, light.dir);
    float spotatten;
    if (spotdot < light.cosCutOff) {
	spotatten = 0.0;
    } else {
	spotatten = pow(spotdot, light.exp);
    }
    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fragPosition.xyz);

    float n_dot_l = max(dot(fragNormal, l), 0.0);
    vec3 diffuse;
    vec3 ambient;
    if (textured && !infrared) {
    	diffuse =  tex * n_dot_l * spotatten;
	ambient = tex;
    } else {
	diffuse =  material.kd.xyz * n_dot_l * spotatten;
	ambient = material.kd.xyz;
    }
    vec3 specular = vec3(0.0);

    if (n_dot_l > 0.0) {
	// Halfway vectorm for BLINN
	vec3 h = normalize(v + l);
        float n_dot_h = max(dot(fragNormal, h), 0.0);

        specular = material.ks * pow(n_dot_h, material.shininess) * spotatten;
    }
    return ambientIntensity*ambient + ambient*light.rgbIntensity * (diffuse + specular);
}

void main() {
	fragColour = vec4(phongModel(fs_in.position_ES, fs_in.normal_ES), 1.0);
	if (infrared) {
		fragColour = vec4(phongModel(fs_in.position_ES, fs_in.normal_ES), material.kd.w);
	}
}
