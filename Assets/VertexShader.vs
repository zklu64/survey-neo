#version 330

// Model-Space coordinates
in vec3 position;
in vec3 normal;
in vec2 tex;

struct SpotLight {
    vec3 position;
    vec3 rgbIntensity;
    float cosCutOff;
    vec3 dir;
    float exp;
};
uniform SpotLight light;

uniform mat4 ModelView;
uniform mat4 Perspective;

// Remember, this is transpose(inverse(ModelView)).  Normals should be
// transformed using this matrix instead of the ModelView matrix.
uniform mat3 NormalMatrix;

out VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
	SpotLight light;
	vec2 tex;
} vs_out;


void main() {
	vec4 pos4 = vec4(position, 1.0);

	//-- Convert position and normal to Eye-Space:
	vs_out.position_ES = (ModelView * pos4).xyz;
	vs_out.normal_ES = normalize(NormalMatrix * normal);
	vs_out.tex = tex;
	vs_out.light = light;
	gl_Position = Perspective * ModelView * vec4(position, 1.0);
}
