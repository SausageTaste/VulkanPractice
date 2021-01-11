#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 v_frag_pos;


layout(push_constant) uniform constants {
    mat4 model_mat;
} c_push_consts;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;


vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);


void main() {
    vec4 world_pos = c_push_consts.model_mat * vec4(inPosition, 1.0);
    v_frag_pos = world_pos.xyz;
    gl_Position = ubo.proj * ubo.view * world_pos;
    v_normal = normalize((c_push_consts.model_mat * vec4(inNormal, 0)).xyz);
    fragTexCoord = inTexCoord;
}
