#version 120

attribute vec3 vert_pos;
attribute mat4 inst_modelmat;

uniform mat4 lightspace;

void main()
{
    gl_Position = lightspace * inst_modelmat * vec4(vert_pos, 1.0);
}
