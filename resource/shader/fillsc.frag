#version 450

#include "pbr_lighting.glsl"


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_position;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_normal;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_albedo;
layout (input_attachment_index = 4, binding = 4) uniform subpassInput input_material;

layout(binding = 5) uniform UniformBufferObject {
    vec4 m_view_pos;

    vec4 m_num_of_plight_dlight_slight;

    vec4 m_plight_color[5];
    vec4 m_plight_pos[5];

    vec4 m_dlight_color[3];
    vec4 m_dlight_direc[3];
} u_per_frame;


layout (location = 0) out vec4 out_color;


vec3 fix_color(vec3 color) {
    const float EXPOSURE = 0.8;

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

    vec3 view_direc = normalize(u_per_frame.m_view_pos.xyz - frag_world_pos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, material.y);
    vec3 light = 0.02 * albedo;

    for (uint i = 0; i < u_per_frame.m_num_of_plight_dlight_slight.x; ++i) {
        vec3 frag_to_light_vec = u_per_frame.m_plight_pos[i].xyz - frag_world_pos;
        light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, normalize(frag_to_light_vec), length(frag_to_light_vec), u_per_frame.m_plight_color[i].xyz);
    }
    for (uint i = 0; i < u_per_frame.m_num_of_plight_dlight_slight.y; ++i) {
        const vec3 frag_to_light_direc = normalize(-u_per_frame.m_dlight_direc[i].xyz);
        light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, frag_to_light_direc, 1, u_per_frame.m_dlight_color[i].xyz);
    }

    out_color.xyz = light;
    out_color.w = 1;

    out_color.xyz = fix_color(out_color.xyz);
}
