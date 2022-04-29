#version 120

attribute vec3 vert_pos;
attribute vec2 vert_uv;

varying vec2 tex_coord;

void main()
{
    tex_coord = vert_uv;
    gl_Position = vec4(vert_pos, 1.0);
}