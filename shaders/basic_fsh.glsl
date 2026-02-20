// Fragment shader for 2D rendering — deko3d / uam
// Compiled by: uam -s frag -o basic_fsh.dksh basic_fsh.glsl
#version 460

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec4 fragColor;

layout (binding = 0) uniform sampler2D tex;

layout (std140, binding = 1) uniform FsUniforms {
    int useTexture;   // 0 = color only, 1 = texture × color
};

layout (location = 0) out vec4 outColor;

void main() {
    if (useTexture != 0) {
        outColor = texture(tex, fragUV) * fragColor;
    } else {
        outColor = fragColor;
    }
}
