#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_position;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_normal;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_albedo;
layout (input_attachment_index = 4, binding = 4) uniform subpassInput input_material;

layout (location = 0) out vec4 out_color;


const vec3 CAMERA_POS = vec3(0, 2, 4);
const vec3 LIGHT_POS = vec3(2, 4, 5);
const vec3 LIGHT_COLOR = vec3(10);
const float FADEOUT_DISTANCE = 5;

const float PI = 3.14159265359;


float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.00001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


vec3 calc_pbr_illumination(float roughness, float metallic, vec3 albedo, vec3 normal, vec3 F0, vec3 view_direc, vec3 frag_to_light_direc, float light_distance, vec3 light_color) {
    // calculate per-light radiance
    vec3 L = frag_to_light_direc;
    vec3 H = normalize(view_direc + L);
    float dist = light_distance;
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = light_color * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, H, roughness);
    float G   = GeometrySmith(normal, view_direc, L, roughness);
    vec3 F    = fresnelSchlick(clamp(dot(H, view_direc), 0.0, 1.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(normal, view_direc), 0.0) * max(dot(normal, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.00001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    float NdotL = max(dot(normal, L), 0.0);

    // add to outgoing radiance Lo
    return (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}

vec3 fix_color(vec3 color) {
    const float EXPOSURE = 0.4;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    return mapped;
}


void main() {
    float depth = subpassLoad(input_depth).x;
    vec3 frag_world_pos = subpassLoad(input_position).xyz;
    vec3 normal = subpassLoad(input_normal).xyz;
    vec3 albedo = subpassLoad(input_albedo).xyz;
    vec2 material = subpassLoad(input_material).xy;

    vec3 view_direc = normalize(CAMERA_POS - frag_world_pos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, material.y);
    vec3 light = 0.1 * albedo;

    light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, normalize(LIGHT_POS), 1, LIGHT_COLOR);

    out_color.xyz = light;
    out_color.w = 1;

    out_color.xyz = fix_color(out_color.xyz);
}
