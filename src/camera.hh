#ifndef CAMERA_H
#define CAMERA_H

#include <windows.h>
#include "third_party/HandmadeMath.h"

struct Camera
{
    HMM_Vec3 cam_pos;
    HMM_Vec3 cam_dir;
    HMM_Vec3 cam_up;
    
    HMM_Mat4 projection;
    HMM_Mat4 transform;

    float yaw;
    float pitch;
};

struct ProjectionInfo
{
    float width;
    float height;
    float fov_rad;
    float near_plane;
};

void CameraSetProjection(Camera *camera, ProjectionInfo proj_info);
void CameraUpdate(Camera *camera, POINT cursor_delta);

#endif //CAMERA_H
