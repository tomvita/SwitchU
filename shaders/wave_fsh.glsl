// Wave/ripple page transition — deko3d / uam
#version 460

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec4 fragColor;

layout (binding = 0) uniform sampler2D tex;

layout (std140, binding = 1) uniform FsUniforms {
    int   useTexture;
    float waveTime;       // 0..1 transition progress
    float waveAmplitude;  // distortion strength (pixels / screenWidth)
    float waveFrequency;  // number of wave cycles
};

layout (location = 0) out vec4 outColor;

void main() {
    // Smooth envelope: strongest in the middle of transition, fades at edges
    float envelope = sin(waveTime * 3.14159);
    float dist = waveAmplitude * envelope;

    // Horizontal wave distortion based on Y coordinate
    float phase = fragUV.y * waveFrequency * 6.28318 - waveTime * 12.0;
    vec2 uv = fragUV;
    uv.x += sin(phase) * dist;

    // Slight vertical wobble too
    float phaseV = fragUV.x * waveFrequency * 0.5 * 6.28318 - waveTime * 8.0;
    uv.y += sin(phaseV) * dist * 0.3;

    outColor = texture(tex, clamp(uv, 0.0, 1.0)) * fragColor;
}
