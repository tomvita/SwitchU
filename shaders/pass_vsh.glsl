// Fullscreen pass vertex shader — deko3d / uam
// Used for post-processing (blur, wave, etc.)
// Same vertex format as basic_vsh but generates fullscreen triangle from vertex ID
#version 460

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;

layout (std140, binding = 0) uniform VsUniforms {
    mat4 projection;
};

layout (location = 0) out vec2 fragUV;
layout (location = 1) out vec4 fragColor;

void main() {
    gl_Position = projection * vec4(inPos, 0.0, 1.0);
    fragUV    = inUV;
    fragColor = inColor;
}
