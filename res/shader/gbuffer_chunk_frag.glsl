#version 120

varying vec3 frag_pos;
varying vec2 tex_coord;
varying vec3 frag_normal;
varying mat3 frag_TBN;

uniform sampler2D albedo_map;
//uniform sampler2D specular_map;
uniform sampler2D normal_map;

/*
    Framebuffers:
        0 = Position
        1 = Normal
        2 = Albedo + Specularity
        3 = Data
*/

void main()
{
    vec4 raw_color = texture2D(albedo_map, tex_coord);

    /* Delete pixel if alpha too low */
    if (raw_color.a < 0.95) {
        discard;
    }
    vec3 color = raw_color.rgb;
    
    vec3 normal = texture2D(normal_map, tex_coord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(frag_TBN * normal);

    gl_FragData[0] = vec4(frag_pos, 1.0);
    gl_FragData[1] = vec4(normal, 1.0);
    gl_FragData[2] = vec4(color, 1.0);
}