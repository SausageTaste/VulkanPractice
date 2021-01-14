#version 450

#include "pbr_lighting.glsl"


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_position;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_normal;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_albedo;
layout (input_attachment_index = 4, binding = 4) uniform subpassInput input_material;

layout(binding = 5) uniform UniformBufferObject {
    vec3 m_view_pos;
} u_per_frame;


layout (location = 0) out vec4 out_color;


vec3 fix_color(vec3 color) {
    const float EXPOSURE = 0.4;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    return mapped;
}


void main() {
    const vec3 LIGHT_POS = vec3(2, 4, 2);
    const vec3 LIGHT_COLOR = vec3(100);

    float depth = subpassLoad(input_depth).x;
    vec3 frag_world_pos = subpassLoad(input_position).xyz;
    vec3 normal = subpassLoad(input_normal).xyz;
    vec3 albedo = subpassLoad(input_albedo).xyz;
    vec2 material = subpassLoad(input_material).xy;

    vec3 view_direc = normalize(u_per_frame.m_view_pos - frag_world_pos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, material.y);
    vec3 light = 0.1 * albedo;

    vec3 frag_to_light_vec = LIGHT_POS - frag_world_pos;
    light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, normalize(frag_to_light_vec), length(frag_to_light_vec), LIGHT_COLOR);

    out_color.xyz = light;
    out_color.w = 1;

    out_color.xyz = fix_color(out_color.xyz);
}
