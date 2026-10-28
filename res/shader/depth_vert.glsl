#version 120

attribute vec3 vert_pos;

uniform mat4 model_mat;
uniform mat4 lightspace;

void main()
{
    gl_Position = lightspace * model_mat * vec4(vert_pos, 1.0);
}
