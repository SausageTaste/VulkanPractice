#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 v_frag_pos;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_albedo;


layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform U_Material {
    float m_roughness;
    float m_metallic;
} u_material;


void main() {
    out_position = vec4(v_frag_pos, 1);
    out_normal = vec4(v_normal, 1);
    out_albedo = texture(texSampler, fragTexCoord);
}
