#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_position;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_normal;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_albedo;

layout (location = 0) out vec4 out_color;


const vec3 BRIGHT_CENTER = vec3(0, 0, 3);
const float FADEOUT_DISTANCE = 5;


float remap_range(float v, float a, float b, float c, float d) {
    float alpha = (b - a) / (d - c);
    float beta = -c * alpha + a;
    return alpha * v + beta;
}


void main() {
    float depth = subpassLoad(input_depth).x;
    vec3 position = subpassLoad(input_position).xyz;
    vec3 normal = subpassLoad(input_normal).xyz;
    vec3 albedo = subpassLoad(input_albedo).xyz;

    float dist_from_origin = distance(position, BRIGHT_CENTER);
    float brightness = clamp(-1.0 / FADEOUT_DISTANCE * dist_from_origin + 1, 0, 1);

    if (2.0 * gl_FragCoord.x < gl_FragCoord.y) {
        out_color = vec4(normal, 1);
    }
    else if (gl_FragCoord.x < gl_FragCoord.y) {
        out_color = vec4(position, 1);
    }
    else if (gl_FragCoord.x < gl_FragCoord.y * 2) {
        float remapped_depth = remap_range(depth, 0, 1, 0.95, 1);
        out_color = vec4(vec3(remapped_depth), 1);
    }
    else {
        out_color = vec4(albedo, 1) * brightness;
    }
}
