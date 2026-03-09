#version 410 core
layout(location = 0) in vec3 vPosition;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    vec3 pos = vPosition;
    float speed = 25.0; // viteza 
    float rangeY = 50.0; // inaltime

    pos.y -= mod(time * speed, rangeY);

    if (pos.y < 0.0) {
        pos.y += rangeY;
    }

    gl_Position = projection * view * vec4(pos, 1.0);
}