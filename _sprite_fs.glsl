#version 330 core
in vec2 vUV;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 color = texture(uTexture, vUV);
    if (color.a < 0.05) discard;
    FragColor = color;
}
