#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_position;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_normal;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_albedo;

layout (location = 0) out vec4 outColor;


void main() {
    const vec3 RED = vec3(1, 0, 0);

    vec3 position = subpassLoad(input_position).xyz;
    outColor = vec4(position.xy, 1, 1);
}
