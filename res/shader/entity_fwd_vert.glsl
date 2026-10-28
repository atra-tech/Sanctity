#version 120

#define MAX_BONES 64

attribute vec3 vert_pos;
attribute vec2 vert_uv;
attribute vec3 vert_norm;
attribute vec4 vert_color;
attribute vec3 vert_tangent;
attribute vec2 vert_uv2;
attribute vec4 vert_boneids;
attribute vec4 vert_boneweights;

varying vec3 frag_pos;
varying vec2 frag_uv;
varying vec2 frag_uv2;
varying vec3 frag_color;
varying vec4 frag_pos_lightspace;
varying mat3 frag_TBN;

uniform mat4 modelmat;
uniform mat4 viewprojmat;
uniform mat4 lightspace;
uniform mat4 bone_mats[MAX_BONES];

mat4 inverse(mat4 m) {
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3];
    float a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3];

    float b00 = a00 * a11 - a01 * a10;
    float b01 = a00 * a12 - a02 * a10;
    float b02 = a00 * a13 - a03 * a10;
    float b03 = a01 * a12 - a02 * a11;
    float b04 = a01 * a13 - a03 * a11;
    float b05 = a02 * a13 - a03 * a12;
    float b06 = a20 * a31 - a21 * a30;
    float b07 = a20 * a32 - a22 * a30;
    float b08 = a20 * a33 - a23 * a30;
    float b09 = a21 * a32 - a22 * a31;
    float b10 = a21 * a33 - a23 * a31;
    float b11 = a22 * a33 - a23 * a32;
    float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
    
    return mat4(
        a11 * b11 - a12 * b10 + a13 * b09,
        a02 * b10 - a01 * b11 - a03 * b09,
        a31 * b05 - a32 * b04 + a33 * b03,
        a22 * b04 - a21 * b05 - a23 * b03,
        a12 * b08 - a10 * b11 - a13 * b07,
        a00 * b11 - a02 * b08 + a03 * b07,
        a32 * b02 - a30 * b05 - a33 * b01,
        a20 * b05 - a22 * b02 + a23 * b01,
        a10 * b10 - a11 * b08 + a13 * b06,
        a01 * b08 - a00 * b10 - a03 * b06,
        a30 * b04 - a31 * b02 + a33 * b00,
        a21 * b02 - a20 * b04 - a23 * b00,
        a11 * b07 - a10 * b09 - a12 * b06,
        a00 * b09 - a01 * b07 + a02 * b06,
        a31 * b01 - a30 * b03 - a32 * b00,
        a20 * b03 - a21 * b01 + a22 * b00) / det;
}

mat4 bone_transform()
{
	mat4 result;
	
	float norm_fac = 1.0 / (vert_boneweights.x + vert_boneweights.y + vert_boneweights.z + vert_boneweights.w);
	result = norm_fac * vert_boneweights.w * bone_mats[int(vert_boneids.w)] +
			 norm_fac * vert_boneweights.z * bone_mats[int(vert_boneids.z)] +
			 norm_fac * vert_boneweights.y * bone_mats[int(vert_boneids.y)] +
			 norm_fac * vert_boneweights.x * bone_mats[int(vert_boneids.x)];
	
	return result;
}

void main()
{
	mat4 bone_tf = bone_transform();
    vec4 pos = bone_tf * vec4(vert_pos, 1.0);
    vec3 norm = mat3(transpose(inverse(bone_tf))) * vert_norm;
    
    vec4 world_pos = modelmat * pos;
    mat3 normal_mat = mat3(transpose(inverse(modelmat)));
    
    frag_pos = world_pos.xyz;
    frag_uv = vert_uv;
    frag_uv2 = vert_uv2;
    frag_color = vert_color.rgb;
    frag_pos_lightspace = lightspace * vec4(frag_pos, 1.0);   

    vec3 T = normalize(normal_mat * vert_tangent);
    vec3 N = normalize(normal_mat * norm);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);
    frag_TBN = TBN;

    gl_Position = viewprojmat * world_pos;
}
