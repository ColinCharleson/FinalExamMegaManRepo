#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;

uniform float u_Filter[9];
uniform vec2 u_PixelSize;

void main() {

    float mdf = 0.1;
    vec2 uv = inUV;

    float noise = (fract(sin(dot(uv, vec2(12.9898,78.233)*2.0)) * 43758.5453));

    vec3 tex = texture(s_Image, uv).rgb;
    
    vec3 color = tex - noise * mdf;

    outColor = color;
}
