#version 120

varying vec3 frag_pos;
varying vec2 frag_uv;
varying vec2 frag_uv2;
varying vec3 frag_color;
varying vec4 frag_pos_lightspace;
varying mat3 frag_TBN;

uniform vec3 view_pos;
uniform vec3 ambient_color;
uniform vec3 sun_color;
uniform vec3 sun_dir;

uniform sampler2D albedo_def;
uniform sampler2D normal_def;
uniform sampler2D specular_def;
uniform sampler2D emission_def;

uniform sampler2D albedo_dmg;
uniform sampler2D normal_dmg;
uniform sampler2D specular_dmg;
uniform sampler2D emission_dmg;

uniform sampler2D blend_map;
uniform sampler2D shadow_map;

const int SHADOW_MAP_SIZE = 2048;
vec3 light_dir = -sun_dir;

float
calc_shadow(vec4 fragpos_lightspace, vec3 normal)
{
    vec3 proj_coords = fragpos_lightspace.xyz / fragpos_lightspace.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    float closest_depth = texture2D(shadow_map, proj_coords.xy).r;
    float current_depth = proj_coords.z;

    float bias = max(0.007 * (1.0 - dot(normal, light_dir)), 0.0007);
    float shadow = 0.0;

    vec2 texel_size = vec2(1.0 / SHADOW_MAP_SIZE);
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcf_depth = texture2D(shadow_map, proj_coords.xy + vec2(x, y) * texel_size).r;
            shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (proj_coords.z > 1.0) {
        shadow = 0.0;
    }

    return shadow;
}

void
main()
{
    float blend_val = texture2D(blend_map, frag_uv2).r;

    vec3 albedo_color = ((texture2D(albedo_def, frag_uv) * (1.0 - blend_val)) + 
                        (texture2D(albedo_dmg, frag_uv) * (blend_val))).rgb;
    vec3 normal_color = ((texture2D(normal_def, frag_uv) * (1.0 - blend_val)) + 
                        (texture2D(normal_dmg, frag_uv) * (blend_val))).rgb;
    vec3 specular_color = ((texture2D(specular_def, frag_uv) * (1.0 - blend_val)) + 
                          (texture2D(specular_dmg, frag_uv) * (blend_val))).rgb;
    vec3 emission_color = ((texture2D(emission_def, frag_uv) * (1.0 - blend_val)) + 
                          (texture2D(emission_dmg, frag_uv) * (blend_val))).rgb;

    normal_color = normalize(normal_color * 2.0 - 1.0);
    normal_color = normalize(frag_TBN * normal_color);
    vec3 ambient = ambient_color;

    /* Calculate diffuse lighting */
    float diff = max(dot(light_dir, normal_color), 0.0);
    vec3 diffuse = diff * sun_color;

    /* Calculate specular lighting */
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 refl_dir = reflect(-light_dir, normal_color);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal_color, halfway_dir), 0.0), 32.0);
    vec3 specular = vec3(0.1) * spec * specular_color;

    float shadow = calc_shadow(frag_pos_lightspace, normal_color);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo_color * frag_color;

    gl_FragColor = vec4(lighting + emission_color, 1.0);
}
