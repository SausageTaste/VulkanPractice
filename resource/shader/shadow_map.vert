#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;


layout(binding = 0) uniform U_PerInst_PerFrame {
    mat4 m_model_mat;
} u_obj_dynamic_data;

layout(binding = 1) uniform U_PerLight_PerFrame {
    mat4 m_light_mat;
} u_light_dynamic_data;


void main() {
    gl_Position = u_light_dynamic_data.m_light_mat * u_obj_dynamic_data.m_model_mat * vec4(inPosition, 1.0);
}
