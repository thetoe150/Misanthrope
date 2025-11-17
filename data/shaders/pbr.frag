#version 450

#define PI 3.1415
#define e 2.71828

layout (set = 0, binding = 0) uniform scene {
	mat4 viewProj;
	vec3 camPos;
	vec3 sunPos;
}u_scene;

layout (set = 0, binding = 1) uniform dynamicLight {
	vec3 lightPos[];
	vec3 lightColor[];
}u_dynamicLight;


layout(set = 1, binding = 3) uniform sampler2D u_normalTexure;
layout(set = 1, binding = 4) uniform sampler2D u_albedoTexure;
layout(set = 1, binding = 5) uniform sampler2D u_metallicTexure;
layout(set = 1, binding = 6) uniform sampler2D u_roughnessTexure;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_tangentFragPos;
layout (location = 2) in vec3 v_tangentLightPos;
layout (location = 3) in vec3 v_tangentLightPos;
layout (location = 4) in vec3 v_normal;

layout (push_contant) uniform pushContant {

}p_const;

// Schlick approximation version of Fresnel
vec3 Fsch(vec3 F0, vec3 metalness, vec3 l, vec3 h) {
	return FO + (1.0 - F0) * pow(clamp(1 - (dot(l, h)), 0.0, 1.0), 5);
};

// Trowbridge-Reitz distribution aka GGX (recommended by Blinn)
float Dggx(float roughness, vec n, vec m) {
	float r2 = roughness * roughness;
	float NdotM = clamp(dot(n, m), 0.0, 1.0);
	float numerator = r2;
	float denominator = PI * pow(1 + (NdotM * NdotM) * (r2 - 1), 2);
	return numerator / denominator;
};

// other NDF:
// Phong distribution - model with micro surface probability and can model uniform distribution
// roughness 0 mean uniform distribution
float Dp(float roughness, vec3 n, vec3 m) {
	float NdotM = dot(n, m);
	float factor = (roughness + 2) / (2 * PI);
	return factor * pow(NdotM, roughness);
}
// Beckmann distribution - model with slope degree and can model super-rough” surfaces
// roughness go beyon 1 and to infinity
float Db(float roughness, vec3 n, vec3 m) {
	float r2 = roughness * roughness; 
	float NdotM = clamp(dot(n, m), 0.0, 1.0);
	float denominator = PI * r2 * pow(NdotM, 4) + 0.001;
	float p = - (1 - pow(NdotM), 2) / (r2 * pow(NdotM), 2);
	float numerator = pow(e, p);
	return numerator / denominator;
};

// Smith function family (The original Smith function was designed for the Beckmann NDF
// generalized the Smith function  match any NDF.)
//// Note that the Schlick approximation to the original Smith geometry function is technically
//// incorrect for use in microfacet BRDFs, since it approximates the wrong version of the function.
//// However, some parameter remapping it can be used as an approximation of the correct function. 
float approximateSchlickGGX(float dot, float roughness) {
	float r = roughness + 1;
	float k = (r*r) / 8.0;

    float numerator   = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

float Gm(vec3 n, vec3 v, vec3 l) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// other Geometry function:
// Cook-Torrance (Blinn-Torrance-Sparrow originality??)
float Gct(vec3 n, vec3 v, vec3 l) {
	vec3 h = normalize(v + l);	
	float VdotH = dot(v, h);
	float NdotV = dot(n, v);
	float NdotL = dot(n, l);
	float NdotH = dot(n, h);

	float first = (2 * NdotH * NdotV) / VdotH;
	float second = (2 * NdotH * NdotL) / VdotH;
	float minimum = min(first, second);
	return min(1, minimum);
};

// Kelemen function (simplified version of Cook-Torrance)
float Gk(vec3 n, vec3 v, vec3 l) {
	vec3 h = normalize(v + l);	
	float NdotV = dot(n, v);
	float NdotL = dot(n, l);
	float LdotH = dot(l, h);

	return 1 / LdotH * LdotH;
	// approximation of: return Gct(n, v, l) / (NdotV * NdotL);
};

// Lambertian BRDF - Subsurface Reflectance / diffuse term
vec3 getDiffuse(vec3 fresnel, float albedo, float metallic) {
	float kD = (vec3(1.0) - fresnel) * (1.0 - metallic);
	float f = albedo / PI;
	return kD * f;
}

vec3 getSpecular(vec3 fresnel, vec3 n, vec3 v, vec3 l, float roughness, float metalic) {
	vec3 numerator = Dggx(roughness, n, h) * Gm(n, v, l) * fresnel;
	float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
	vec3 spec = numerator / denominator; 
}

void main() {
	vec3 n = v_normal;
	vec3 l_sun = u_scene.sunPos - v_tangentFragPos;
	vec3 h_sun = normalize(v + l);
	vec3 v = u_scene.camPos - v_tangentFragPos;

	float albedo = texture(u_albedoTexure, v_uv).r;
	float metallic = texture(u_metalicTexure, v_uv).r;
	float roughness = texture(u_roughnessTexure, v_uv).r;


	float F0 = 0.03;
	F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
	for (int i = 0; i < LIGHT_COUNT; i++) {
		vec3 l = u_dynamicLight.lightPos[i] - v_tangentFragPos;
		vec3 h = normalize(v + l);
		float distance = length(u_dynamicLight.lightPos[i] - v_tangentFragPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = u_dynamicLight.lightColor[i] * attenuation;

        vec3 fresnel = Fsch(F0, metallic, l, h);
		float NdotL = max(dot(n, l), 0);
		vec3 diffuse = getDiffuse(fresnel, albedo, metallic);
		vec3 specular = getSpecular(fresnel, n, v, l, roughness, metallic);
		Lo += (diffuse + specular) * NdotL * radiance;
	}

    vec3 ambient = vec3(0.03) * albedo * ao;
	vec3 color = ambient + Lo;
    // HDR tonemapping
	color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}
