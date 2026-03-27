// Animated gradient background — deko3d / uam
// Creates organic, slowly-moving color blobs using simplex-like noise
#version 460

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec4 fragColor;

layout (binding = 0) uniform sampler2D tex;

layout (std140, binding = 1) uniform FsUniforms {
    int   useTexture;
    float time;
    float pad1;
    float pad2;
    vec4  colorA;    // primary gradient color
    vec4  colorB;    // secondary gradient color
    vec4  colorC;    // accent blob color
};

layout (location = 0) out vec4 outColor;

// Simple hash-based noise (no texture lookups needed)
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x * 34.0) + 1.0) * x); }

// 2D simplex noise
float snoise(vec2 v) {
    const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                       -0.577350269189626, 0.024390243902439);
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);
    vec2 i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;
    i = mod289(i);
    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0));
    vec3 m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0);
    m = m * m;
    m = m * m;
    vec3 x_  = 2.0 * fract(p * C.www) - 1.0;
    vec3 h   = abs(x_) - 0.5;
    vec3 ox  = floor(x_ + 0.5);
    vec3 a0  = x_ - ox;
    m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);
    vec3 g;
    g.x = a0.x * x0.x + h.x * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

void main() {
    vec2 uv = fragUV;
    float t = time * 0.15; // slow movement

    // Multiple noise octaves for organic motion
    float n1 = snoise(uv * 2.0 + vec2(t * 0.7, t * 0.5));
    float n2 = snoise(uv * 3.0 + vec2(-t * 0.4, t * 0.8));
    float n3 = snoise(uv * 1.5 + vec2(t * 0.3, -t * 0.6));

    // Base gradient (vertical)
    float grad = uv.y;

    // Warp the gradient with noise
    grad += n1 * 0.15 + n2 * 0.08;
    grad = clamp(grad, 0.0, 1.0);

    // Blend between primary colors
    vec3 color = mix(colorA.rgb, colorB.rgb, grad);

    // Add accent color blobs
    float blob1 = smoothstep(0.0, 0.6, n1 * 0.5 + 0.5);
    float blob2 = smoothstep(0.1, 0.7, n3 * 0.5 + 0.5);
    color = mix(color, colorC.rgb, blob1 * 0.15);
    color = mix(color, mix(colorA.rgb, colorC.rgb, 0.5), blob2 * 0.10);

    // Subtle vignette
    vec2 vig = uv - 0.5;
    float vigFactor = 1.0 - dot(vig, vig) * 0.3;
    color *= vigFactor;

    outColor = vec4(color, 1.0) * fragColor;
}
