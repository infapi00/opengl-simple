#version 310 es

in vec2 attr_pos;

void main() {
   gl_Position = vec4(attr_pos, 1.0f, 1.0f);
}
