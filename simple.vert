#version 120

attribute vec2 attr_pos;

void main() {
   gl_Position = vec4(attr_pos, 0.0f, 1.0f);
}
