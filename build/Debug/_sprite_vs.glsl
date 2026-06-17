#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

uniform mat4 uProjection;
uniform mat4 uModel;
uniform vec2 uUVOffset;
uniform vec2 uUVSize;

void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
    vUV = uUVOffset + aUV * uUVSize;
}
