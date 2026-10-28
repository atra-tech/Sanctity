#version 120

varying vec2 frag_uv;
varying vec3 frag_norm;
varying vec4 frag_color;
varying vec4 frag_pos_lightspace;

uniform vec3 view_pos;
uniform vec3 ambient_color;
uniform vec3 sun_color;
uniform vec3 sun_dir;

uniform sampler2D albedo_map;
uniform sampler2D emission_map;
uniform sampler2D shadow_map;

vec3 light_dir = -sun_dir;

float
calc_shadow(vec4 fragpos_lightspace, vec3 normal)
{
    vec3 proj_coords = fragpos_lightspace.xyz / fragpos_lightspace.w;
    proj_coords = proj_coords * 0.5 + 0.5;

    float closest_depth = texture2D(shadow_map, proj_coords.xy).r;
    float current_depth = proj_coords.z;

    float bias = max(0.005 * (1.0 - dot(normal, light_dir)), 0.0005);
    float shadow = 0.0;

    vec2 texel_size = vec2(1.0 / 2048);
    
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

void main()
{
    vec4 albedo_sample = texture2D(albedo_map, frag_uv);
    vec3 emission_color = texture2D(emission_map, frag_uv).rgb;
    
    vec3 albedo_color = albedo_sample.rgb * frag_color.rgb;
    float alpha = albedo_sample.a * frag_color.a;

    if (alpha < 0.5f) {
        discard;
    }

    /* Calculate diffuse lighting */
    float diff = max(dot(light_dir, frag_norm), 0.0);
    vec3 diffuse = diff * sun_color;

    float shadow = calc_shadow(frag_pos_lightspace, frag_norm);
    vec3 lighting = /*(ambient_color + (1.0 - shadow) * diffuse) **/ albedo_color;

    gl_FragColor = vec4(lighting + emission_color, alpha);
}
