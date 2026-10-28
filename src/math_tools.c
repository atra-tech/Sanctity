/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  math_tools.c - Supplementary mathematical utilities
 *
 */

#include "math_tools.h"

/* Linear interpolation between a and b by t (corrects for a > b) */
float
lerpf(float a, float b, float t)
{
    t = CLAMP(t, 0.0f, 1.0f);

    return ((a > b) ? (b + (a - b) * (1.0 - t)) : (a + (b - a) * t));
}

/* Takes the bounds of a 3D AABB, and returns a random point in `dest` */
void
rand_getuniformpoint_aabb(const float *min, const float *max, float *dest)
{
    dest[0] = atra_util_randomfloat(min[0], max[0]);
    dest[1] = atra_util_randomfloat(min[1], max[1]);
    dest[2] = atra_util_randomfloat(min[2], max[2]);
}

/* Takes a 3D position and radius and returns a random point in `dest`.
 * Guarantees uniform point distribution. */
void
rand_getuniformpoint_sphere(const float *pos, float radius, float *dest)
{
    const vec3 min = {
        pos[0] - radius,
        pos[1] - radius,
        pos[2] - radius
    };

    const vec3 max = {
        pos[0] + radius,
        pos[1] + radius,
        pos[2] + radius
    };

    float sq_dist = radius * radius + 1.0f;
    vec3 diff;

    while (sq_dist > radius * radius) {
        rand_getuniformpoint_aabb(min, max, dest);
        diff[0] = dest[0] - pos[0];
        diff[1] = dest[1] - pos[1];
        diff[2] = dest[2] - pos[2];
        sq_dist = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
    }
}
/*
    OLD CONE DIRECTION DEVIATION ALGORITHM

    if (e->spread > 0.0f) {
            theta = atra_util_randomfloat(0.0f, 2 * PI);
            phi = atra_util_randomfloat(0.0f, PI);
            radius = atra_util_randomfloat(0.0f, e->spread);

            sphere_point[0] = radius * sinf(phi) * cosf(theta);
            sphere_point[1] = radius * sinf(phi) * sinf(theta);
            sphere_point[2] = radius * cosf(phi);

            spread_radius = tanf(e->spread / 2.0f);

            memcpy(dir, e->dir, sizeof(vec3));
            glm_vec3_muladds(sphere_point, spread_radius, dir);

            particlelist_add(plist, e->pos, rot, dir, plist->lifetime, plist->gravity);
        } 
*/

/* Calculates a random 3D rotation vector */
void
rand_rotation(float *rot)
{
    rot[0] = atra_util_randomfloat(0, 2 * PI);
    rot[1] = atra_util_randomfloat(0, 2 * PI);
    rot[2] = atra_util_randomfloat(0, 2 * PI);
}

/* Calculates a random 3D rotation vector and it's corresponding direction normal */
void
rand_rotation_dir(float *rot, float *dir)
{
    rand_rotation(rot);
    
    dir[0] = cosf(rot[0]) * sinf(rot[1]);
    dir[1] = sinf(rot[0]);
    dir[2] = cosf(rot[0]) * cosf(rot[1]);
}

/* Offset a given directional vector randomly within a given fan angle (vec2) */
void
rand_dir_vec2_fan(float *dir, float spread, float *dest)
{
    float theta = atra_util_randomfloat(-spread, spread);

    dest[0] = dir[0] * cosf(theta) - dir[1] * sinf(theta);
    dest[1] = dir[0] * sinf(theta) + dir[1] * cosf(theta);
}

/* Returns a random, normal direction vector at any point on a circle (vec2) */
void
rand_dir_vec2_circle(float *dest)
{
    float theta = atra_util_randomfloat(0.0f, 2 * PI);

    dest[0] = cosf(theta);
    dest[1] = sinf(theta);
}

/* Offset a given directional vector randomly with in a given conical angle */
void
rand_dir_vec3_cone(float *dir, float spread, float *dest)
{
    float theta = atra_util_randomfloat(-spread, spread);
    float phi = atra_util_randomfloat(0.0f, PI);

    versor tilt, azimuth;
    vec3 right;
   
    rand_getrightvec_any(dir, right);
    glm_quat(tilt, theta, right[0], right[1], right[2]);
    glm_quat(azimuth, phi, dir[0], dir[1], dir[2]);
    glm_quat_rotatev(tilt, dir, dest);
    glm_quat_rotatev(azimuth, dest, dest);
}

/* Returns a random, normal direction vector at any point on a sphere */
void
rand_dir_vec3_sphere(float *dest)
{
/*    const vec3 pos = {0.0f};
    const float r = 1.0f;
    
    rand_getuniformpoint_sphere(pos, r, dest);
    glm_vec3_normalize(dest);*/

    float theta_a = atra_util_randomfloat(0.0f, 2 * PI);
    float theta_b = atra_util_randomfloat(0.0f, 2 * PI);

    dest[0] = cosf(theta_a) * sinf(theta_b);
    dest[1] = sinf(theta_a);
    dest[2] = cosf(theta_a) * cosf(theta_b);
}

/* Returns some undefinted right vector, guarantees axis safety */
void
rand_getrightvec_any(float *dir, float *dest)
{
    if (dir[0] == 1.0f) {
        rand_getrightvec_yperp(dir, dest);
    } else {
        rand_getrightvec_xperp(dir, dest);
    }
}

/* Returns an Y-perpendicular right vector for a given dir vector */
void
rand_getrightvec_xperp(float *dir, float *dest)
{
    vec3 x_dir = {1.0f, 0.0f, 0.0f};

    glm_vec3_cross(dir, x_dir, dest);
}

/* Returns an Y-perpendicular right vector for a given dir vector */
void
rand_getrightvec_yperp(float *dir, float *dest)
{
    vec3 y_dir = {0.0f, 1.0f, 0.0f};
    
    glm_vec3_cross(dir, y_dir, dest);
}

