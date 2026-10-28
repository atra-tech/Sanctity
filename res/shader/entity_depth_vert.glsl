#version 120

#define MAX_BONES 32

attribute vec3 vert_pos;
attribute vec4 vert_boneids;
attribute vec4 vert_boneweights;

uniform mat4 modelmat;
uniform mat4 lightspace;
uniform mat4 bone_mats[MAX_BONES];

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
    vec4 pos = bone_transform() * vec4(vert_pos, 1.0);

    gl_Position = lightspace * modelmat * pos;
}
