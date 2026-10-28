#version 120

attribute vec3 vert_pos;
attribute vec3 vert_norm;
attribute mat4 inst_modelmat;
attribute vec2 inst_uvs_0;
attribute vec2 inst_uvs_1;
attribute vec2 inst_uvs_2;
attribute vec2 inst_uvs_3;
attribute vec4 inst_color;

varying vec2 frag_uv;
varying vec3 frag_norm;
varying vec4 frag_color;
varying vec4 frag_pos_lightspace;

uniform mat4 viewproj;
uniform mat4 lightspace;

void main()
{
    vec4 world_pos = inst_modelmat * vec4(vert_pos, 1.0);
    frag_pos_lightspace = lightspace * vec4(world_pos.xyz, 1.0);
    frag_norm = vert_norm;
    frag_color = inst_color;

    /* Calculate which UV coordinate to use without vertex IDs */ 
    if (vert_pos.x > 0.0) {
        if (vert_pos.y > 0.0) {
            frag_uv = inst_uvs_3;
        } else {
            frag_uv = inst_uvs_2;
        }
    } else {
        if (vert_pos.y > 0.0) {
            frag_uv = inst_uvs_0;
        } else {
            frag_uv = inst_uvs_1;
        }
    }

    gl_Position = viewproj * world_pos;
}
