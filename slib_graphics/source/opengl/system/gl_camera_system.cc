#include "gl_camera_system.h"
#include "entity_manager.h"
#include "matrix4x4.h"
#include "quaternion.h"

namespace lib_graphics {
void GlCameraSystem::LogicUpdate(float dt) {
  auto camera_comps = g_ent_mgr.GetNewCbt<Camera>();

  if (camera_comps) {
    auto camera_comps_old = g_ent_mgr.GetOldCbt<Camera>();
    auto camera_update = g_ent_mgr.GetNewUbt<Camera>();

    for (int i = 0; i < camera_update->size(); ++i) {
      if (!(*camera_update)[i]) continue;
      (*camera_update)[i] = false;

      auto ent_vec = g_ent_mgr.GetEbt<Camera>();
      auto px_character =
          g_ent_mgr.GetNewCbeR<lib_physics::Character>(ent_vec->at(i));

      UpdateCamera((*camera_comps)[i], (*camera_comps_old)[i], px_character);
      (*camera_comps_old)[i] = (*camera_comps)[i];
    }
  }
}

void GlCameraSystem::UpdateCamera(Camera &cam, Camera &old,
                                  const lib_physics::Character *actor) {
  if (!cam.set_flags_[0] && !old.set_flags_[0]) {
    cam.rotation_ = old.rotation_ + cam.delta_rot_ + old.delta_rot_;
  } else {
    cam.rotation_ = old.delta_rot_;
    cam.set_flags_[0] = false;
  }

  if (!cam.set_flags_[1])
    cam.exposure_ = old.exposure_;
  else
    cam.set_flags_[1] = false;

  if (!cam.set_flags_[2])
    cam.fov_ = old.fov_;
  else
    cam.set_flags_[2] = false;

  if (!cam.set_flags_[3])
    cam.a_ratio_ = old.a_ratio_;
  else
    cam.set_flags_[3] = false;

  if (!cam.set_flags_[4])
    cam.near_ = old.near_;
  else
    cam.set_flags_[4] = false;

  if (!cam.set_flags_[5])
    cam.far_ = old.far_;
  else
    cam.set_flags_[5] = false;

  cam.delta_rot_.ZeroMem();
  old.delta_pos_.ZeroMem();

  if (!actor)
    cam.position_ = old.position_ + cam.delta_pos_ + old.delta_pos_;
  else
    cam.position_ = actor->pos;

  cam.delta_pos_.ZeroMem();
  old.delta_pos_.ZeroMem();

  float pi2 = PI * 2;
  for (int i = 0; i < 3; ++i) {
    if (cam.rotation_[i] > pi2)
      cam.rotation_[i] -= pi2;
    else if (cam.rotation_[i] < -pi2)
      cam.rotation_[i] += pi2;
  }

  if (cam.rotation_[0] > PI * 0.49f) cam.rotation_[0] = PI * 0.49f;
  if (cam.rotation_[0] < -PI * 0.49f) cam.rotation_[0] = -PI * 0.49f;

  cam.forward_[0] = 0.0f, cam.forward_[1] = 0.0f, cam.forward_[2] = 1.0f;
  cam.up_[0] = 0.0f, cam.up_[1] = 1.0f, cam.up_[2] = 0.0f;
  cam.left_[0] = -1.0f, cam.left_[1] = 0.0f, cam.left_[2] = 0.0f;

  lib_core::Quaternion q;
  q.FromAngle(cam.rotation_);
  q.RotateVector(cam.forward_);
  q.RotateVector(cam.up_);
  q.RotateVector(cam.left_);

  auto orbit = (cam.forward_ * -1.) * cam.orbit_;
  cam.view_.Lookat(cam.position_ + orbit, cam.position_ + cam.forward_,
                   cam.up_);
  cam.proj_.Perspective(cam.fov_ * (PI / 180.0f), cam.a_ratio_, cam.near_,
                        cam.far_);

  cam.view_proj_ = cam.proj_ * cam.view_;
  auto view_mat_ptr = cam.view_proj_.data;

  ExtractPlane(cam.planes_.planes[0], view_mat_ptr, 1);
  ExtractPlane(cam.planes_.planes[1], view_mat_ptr, -1);
  ExtractPlane(cam.planes_.planes[2], view_mat_ptr, 2);
  ExtractPlane(cam.planes_.planes[3], view_mat_ptr, -2);
  ExtractPlane(cam.planes_.planes[4], view_mat_ptr, 3);
  ExtractPlane(cam.planes_.planes[5], view_mat_ptr, -3);
}

void GlCameraSystem::ExtractPlane(Camera::Plane &plane, float *mat, int row) {
  int scale = (row < 0) ? -1 : 1;

  row = abs(row) - 1;

  plane.normal[0] = mat[3] + scale * mat[row];
  plane.normal[1] = mat[7] + scale * mat[row + 4];
  plane.normal[2] = mat[11] + scale * mat[row + 8];
  plane.w = mat[15] + scale * mat[row + 12];

  float length = sqrtf(plane.normal[0] * plane.normal[0] +
                       plane.normal[1] * plane.normal[1] +
                       plane.normal[2] * plane.normal[2]);

  for (int i = 0; i < 3; ++i) plane.normal[i] /= length;
  plane.w /= length;
}
}  // namespace lib_graphics
