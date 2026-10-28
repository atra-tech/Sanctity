/*
 *  SANCTITY
 *
 *  Copyright (c) 2022-2023 Robert Saarloos, All rights reserved.
 *
 *  math_tools.h - Supplementary mathematical utilities
 *
 */

#ifndef MATH_TOOLS_H
#define MATH_TOOLS_H

#include "atra.h"

/* Linear interpolation between a and b by t (corrects for a > b) */
float lerpf(float a, float b, float t);

/* Takes the bounds of a 3D AABB, and returns a random point in `dest` */
void rand_getuniformpoint_aabb(const float *min, const float *max, float *dest);

/* Takes a 3D position and radius and returns a random point in `dest`.
 * Guarantees uniform point distribution. */
void rand_getuniformpoint_sphere(const float *pos, float radius, float *dest);

/* Calculates a random 3D rotation vector */
void rand_rotation(float *rot);

/* Calculates a random 3D rotation vector and it's corresponding direction normal */
void rand_rotation_dir(float *rot, float *dir);

/* Offset a given directional vector randomly within a given fan angle (vec2) */
void rand_dir_vec2_fan(float *dir, float spread, float *dest);

/* Returns a random, normal direction vector at any point in a circle (vec2) */
void rand_dir_vec2_circle(float *dest);

/* Offset a given directional vector randomly within a given conical angle */
void rand_dir_vec3_cone(float *dir, float spread, float *dest);

/* Returns a random, normal direction vector at any point in a sphere */
void rand_dir_vec3_sphere(float *dest);

/* Returns some undefinted right vector, guarantees axis safety */
void rand_getrightvec_any(float *dir, float *dest);

/* Returns an Y-perpendicular right vector for a given dir vector */
void rand_getrightvec_xperp(float *dir, float *dest);

/* Returns an Y-perpendicular right vector for a given dir vector */
void rand_getrightvec_yperp(float *dir, float *dest);


#endif /* MATH_TOOLS_H */
