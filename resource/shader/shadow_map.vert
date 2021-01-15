#version 450


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;


layout(push_constant) uniform constants {
    mat4 transform_mat;
} c_push_consts;


void main() {
    gl_Position = c_push_consts.transform_mat * vec4(inPosition, 1.0);
}
