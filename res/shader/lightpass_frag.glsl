#version 120

varying vec2 tex_coord;

uniform sampler2D position_map;
uniform sampler2D normal_map;
uniform sampler2D albedo_map;

uniform vec3 view_pos;

const vec3 light_pos = vec3(-3.0, 64.0, -3.0);
const vec3 light_color = vec3(1.0, 0.9, 0.6);
const float linear = 1.0;
const float quadratic = 0.0000001;

void main()
{
    vec3 frag_pos = texture2D(position_map, tex_coord).rgb;
    vec3 normal = texture2D(normal_map, tex_coord).rgb;
    vec3 albedo = texture2D(albedo_map, tex_coord).rgb;

    vec3 ambient = albedo * 0.3;
    
    float distance = length(light_pos - frag_pos);
    float attenuation = 1.0 / (linear + quadratic * pow(distance, 4));

    vec3 light_pow = vec3(3.0);

    vec3 light_dir = normalize(light_pos - frag_pos);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * albedo * light_color * attenuation * light_pow;


    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 refl_dir = reflect(-light_dir, normal);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), 32.0);
    vec3 specular = vec3(0.1) * spec * attenuation * light_pow;

    gl_FragColor = vec4(ambient + diffuse + specular, 1.0);
}