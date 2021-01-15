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
    mat4 m_dlight_mat[3];

    vec4 m_slight_pos[5];
    vec4 m_slight_direc[5];
    vec4 m_slight_color[5];
    vec4 m_slight_fade_start_end[5];
} u_per_frame;

layout(binding = 6) uniform sampler2D u_dlight_shadow_map;


layout (location = 0) out vec4 out_color;



float _sample_dlight_0_depth(vec2 coord) {
    if (coord.x > 1.0 || coord.x < 0.0) return 1.0;
    if (coord.y > 1.0 || coord.y < 0.0) return 1.0;
    return texture(u_dlight_shadow_map, coord).r;
}

bool is_frag_in_dlight_0_shadow(vec3 frag_pos) {
    const vec4 frag_pos_in_dlight = u_per_frame.m_dlight_mat[0] * vec4(frag_pos, 1);
    const vec3 projCoords = frag_pos_in_dlight.xyz / frag_pos_in_dlight.w;

    if (projCoords.z > 1.0)
        return false;

    const vec2 sample_coord = projCoords.xy * 0.5 + 0.5;
    const float closestDepth = _sample_dlight_0_depth(sample_coord);
    const float currentDepth = projCoords.z;

    return currentDepth > closestDepth;
}




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
    {
        const vec3 frag_to_light_direc = normalize(-u_per_frame.m_dlight_direc[0].xyz);
        const bool in_shadow = is_frag_in_dlight_0_shadow(frag_world_pos);
        light += in_shadow ? vec3(0) : calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, frag_to_light_direc, 1, u_per_frame.m_dlight_color[0].xyz);
    }
    for (uint i = 1; i < u_per_frame.m_num_of_plight_dlight_slight.y; ++i) {
        const vec3 frag_to_light_direc = normalize(-u_per_frame.m_dlight_direc[i].xyz);
        light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, frag_to_light_direc, 1, u_per_frame.m_dlight_color[i].xyz);
    }
    for (uint i = 0; i < u_per_frame.m_num_of_plight_dlight_slight.z; ++i) {
        const vec3 frag_to_light_vec = u_per_frame.m_slight_pos[i].xyz - frag_world_pos;
        const float attenuation = calc_slight_attenuation(
            frag_world_pos,
            u_per_frame.m_slight_pos[i].xyz,
            u_per_frame.m_slight_direc[i].xyz,
            u_per_frame.m_slight_fade_start_end[i].x,
            u_per_frame.m_slight_fade_start_end[i].y
        );
        light += calc_pbr_illumination(material.x, material.y, albedo, normal, F0, view_direc, normalize(frag_to_light_vec), length(frag_to_light_vec), u_per_frame.m_slight_color[i].xyz) * attenuation;
    }

    out_color.xyz = light;
    out_color.w = 1;

    out_color.xyz = fix_color(out_color.xyz);
}
