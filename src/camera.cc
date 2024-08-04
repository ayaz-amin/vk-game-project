#include <windows.h>
#include "camera.hh"
#include "third_party/HandmadeMath.h"

void CameraSetProjection(Camera *camera, ProjectionInfo proj_info)
{
    HMM_Mat4 *proj = &camera->projection;
    float aspect = proj_info.width / proj_info.height;
    
    float Cotangent = 1.0f / HMM_TanF(proj_info.fov_rad * 0.5f);
    proj->Elements[0][0] = Cotangent / aspect;
    proj->Elements[1][1] = -Cotangent;
    proj->Elements[2][3] = -1.0f;
    proj->Elements[3][2] = proj_info.near_plane;
}

void CameraUpdate(Camera *camera, POINT cursor_delta)
{
    const float cam_speed = 40 * 1.f/60;
    
    if(GetKeyState(0x57) & 0x8000)
    {
        camera->cam_pos += cam_speed * camera->cam_dir;
    }

    if(GetKeyState(0x53) & 0x8000)
    {
        camera->cam_pos -= cam_speed * camera->cam_dir;
    }

    if(GetKeyState(0x41) & 0x8000)
    {
        camera->cam_pos -= cam_speed * HMM_NormV3(HMM_Cross(camera->cam_dir,
                                                            camera->cam_up));
    }

    if(GetKeyState(0x44) & 0x8000)
    {
        camera->cam_pos += cam_speed * HMM_NormV3(HMM_Cross(camera->cam_dir,
                                                            camera->cam_up));
    }

    camera->cam_pos.Y = 1;

    camera->yaw += 0.005 * cursor_delta.x;
    camera->pitch -= 0.005 * cursor_delta.y;

    camera->pitch = HMM_Clamp(-1.567, camera->pitch, 1.567);

    float sin_yaw = HMM_SinF(camera->yaw);
    float cos_yaw = HMM_CosF(camera->yaw);
    float sin_pitch = HMM_SinF(camera->pitch);
    float cos_pitch = HMM_CosF(camera->pitch);

    HMM_Vec3 direction = {};
    direction.X = cos_yaw * cos_pitch;
    direction.Y = sin_pitch;
    direction.Z = sin_yaw * cos_pitch;

    camera->cam_dir = HMM_NormV3(direction);

    HMM_Mat4 view = HMM_LookAt_RH(camera->cam_pos,
                                  camera->cam_pos +
                                  camera->cam_dir,
                                  camera->cam_up);
    
    camera->transform = camera->projection * view;
}
