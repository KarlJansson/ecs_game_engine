#include "light_system.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "light.h"
#include "transform.h"

namespace lib_graphics {
void LightSystem::LogicUpdate(float dt) {
  auto lights = g_ent_mgr.GetNewCbt<Light>();

  if (lights) {
    auto lights_old = g_ent_mgr.GetOldCbt<Light>();
    auto lights_ents = g_ent_mgr.GetEbt<Light>();
    auto light_update = g_ent_mgr.GetNewUbt<Light>();

    auto light_thread = [&](tbb::blocked_range<size_t>& range) {
      for (size_t i = range.begin(); i != range.end(); ++i) {
        if ((*light_update)[i]) {
          auto& current_light = lights->at(i);
          auto& old_light = lights_old->at(i);
          auto old_transform =
              g_ent_mgr.GetNewCbeR<lib_graphics::Transform>(lights_ents->at(i));

          current_light.color = old_light.color;

          if (old_transform)
            current_light.data_pos = old_transform->Position();
          else
            current_light.data_pos = old_light.data_pos + old_light.delta_pos;

          if (old_light.update_cast_shadow) {
            current_light.cast_shadows = old_light.new_cast_shadows;
            old_light.update_cast_shadow = false;
          } else
            current_light.cast_shadows = old_light.cast_shadows;

          for (int ii = 0; ii < 3; ++ii) {
            current_light.shadow_resolutions[ii] =
                old_light.shadow_resolutions[ii];

            if (current_light.shadow_resolutions[ii] >
                g_settings.MaxShadowTexture()) {
              current_light.shadow_resolutions[ii] =
                  g_settings.MaxShadowTexture();
            }
          }

          old_light.delta_pos.ZeroMem();
          (*light_update)[i] = false;
        }
      }
    };

    tbb::parallel_for(tbb::blocked_range<size_t>(0, light_update->size()),
                      light_thread);
  }
}

ct::dyn_array<lib_core::Matrix4x4> LightSystem::GetShadowMatrices(
    Light& light, bool culling) {
  ct::dyn_array<lib_core::Matrix4x4> shadow_transforms;
  if (light.type == Light::kPoint) {
    lib_core::Matrix4x4 shadow_proj, look_at;
    shadow_proj.Perspective(PI / 2.0f, 1.0f, .1f, light.max_radius);

    lib_core::Vector3 light_pos = light.data_pos;
    ct::dyn_array<lib_core::Vector3> forward_vecs = {
        {1.0f, 0.0f, 0.0f},  {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f, -1.0f}};
    ct::dyn_array<lib_core::Vector3> up_vecs = {
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};

    for (int i = 0; i < 6; ++i) {
      shadow_transforms.push_back(shadow_proj);
      look_at.Lookat(light_pos, light_pos + forward_vecs[i], up_vecs[i]);
      shadow_transforms.back() *= look_at;
    }
  } else if (light.type == Light::kDir) {
    CalculateDirLightCascades(light, shadow_transforms, culling);
    shadow_transforms.pop_back();
    shadow_transforms.pop_back();
    shadow_transforms.pop_back();

    Camera* old_cam;
    old_cam = &g_ent_mgr.GetOldCbt<Camera>()->at(0);

    float near_plane = 1.0f, far_plane = 350.0f;
    lib_core::Matrix4x4 light_proj, look_at;

    auto light_pos = old_cam->position_;
    light_pos[1] = 0.f;
    light_pos += light.data_dir * -350.f;
    look_at.Lookat(light_pos, light_pos + light.data_dir,
                   lib_core::Vector3(0.0f, 1.0f, 0.0f));

    light_proj.Orthographic(-60.f, 60.f, -60.0f, 60.0f, near_plane, far_plane);
    shadow_transforms.push_back(light_proj);
    shadow_transforms.back() *= look_at;
    light_proj.Orthographic(-145.f, 145.f, -145.0f, 145.0f, near_plane,
                            far_plane);
    shadow_transforms.push_back(light_proj);
    shadow_transforms.back() *= look_at;
    light_proj.Orthographic(-500.f, 500.f, -500.0f, 500.0f, near_plane,
                            far_plane);
    shadow_transforms.push_back(light_proj);
    shadow_transforms.back() *= look_at;
  }
  return shadow_transforms;
}

void LightSystem::CalculateDirLightCascades(
    Light& light, ct::dyn_array<lib_core::Matrix4x4>& shadow_cascades,
    bool culling) {
  Camera* old_cam;
  old_cam = &g_ent_mgr.GetOldCbt<Camera>()->at(0);

  std::array<Camera::FrustumInfo, 3> f;
  for (auto& i : f) {
    i.fov = old_cam->fov_ * (PI / 180.f) + .2f;
    i.ratio = old_cam->a_ratio_;
  }

  int num_cascades = 3;
  float lambda = .85f;
  float ratio = old_cam->far_ / old_cam->near_;
  f[0].neard = old_cam->near_;

  for (int i = 1; i < num_cascades; i++) {
    float si = i / float(num_cascades);

    f[i].neard =
        lambda * (old_cam->near_ * powf(ratio, si)) +
        (1 - lambda) * (old_cam->near_ + (old_cam->far_ - old_cam->near_) * si);
    f[i - 1].fard = f[i].neard * 1.005f;
  }
  f[num_cascades - 1].fard = old_cam->far_;

  for (int i = 0; i < num_cascades; ++i) {
    lib_core::Vector3 up(0.0f, 1.0f, 0.0f);
    lib_core::Vector3 right = old_cam->forward_;
    right = right.Cross(up);

    lib_core::Vector3 fc = old_cam->position_ + old_cam->forward_ * f[i].fard;
    lib_core::Vector3 nc = old_cam->position_ + old_cam->forward_ * f[i].neard;

    right.Normalize();
    up = right;
    up = up.Cross(old_cam->forward_);
    up.Normalize();

    // these heights and widths are half the heights and widths of
    // the near and far plane rectangles
    float near_height = tanf(f[i].fov * .5f) * f[i].neard;
    float near_width = near_height * f[i].ratio;
    float far_height = tanf(f[i].fov * .5f) * f[i].fard;
    float far_width = far_height * f[i].ratio;

    f[i].point[0] = nc - up * near_height - right * near_width;
    f[i].point[1] = nc + up * near_height - right * near_width;
    f[i].point[2] = nc + up * near_height + right * near_width;
    f[i].point[3] = nc - up * near_height + right * near_width;

    f[i].point[4] = fc - up * far_height - right * far_width;
    f[i].point[5] = fc + up * far_height - right * far_width;
    f[i].point[6] = fc + up * far_height + right * far_width;
    f[i].point[7] = fc - up * far_height + right * far_width;

    lib_core::Matrix4x4 lookat;
    auto dir_normalized = light.data_dir * -1.f;
    dir_normalized.Normalize();
    lookat.Lookat(
        f[i].point[0].Midpoint(f[i].point[7]) + dir_normalized * 100.f,
        light.data_dir, lib_core::Vector3(-1.f, 0.f, 0.f));

    ApplyCropMatrix(f[i], *old_cam, shadow_cascades);
    shadow_cascades.back() *= lookat;

    if (culling)
      light.view_depth[i] =
          .5f *
              (-f[i].fard * old_cam->proj_.data[10] + old_cam->proj_.data[14]) /
              f[i].fard +
          .5f;
  }
}

float LightSystem::ApplyCropMatrix(
    Camera::FrustumInfo& f, Camera& cam,
    ct::dyn_array<lib_core::Matrix4x4>& shadow_cascades) {
  lib_core::Matrix4x4 shad_proj;
  lib_core::Matrix4x4 shad_crop;
  lib_core::Matrix4x4 shad_mvp;
  float maxX = -1000.0f;
  float maxY = -1000.0f;
  float maxZ;
  float minX = 1000.0f;
  float minY = 1000.0f;
  float minZ;

  lib_core::Matrix4x4 shad_modelview = cam.view_;
  lib_core::Matrix4x4 nv_mvp = cam.view_;
  lib_core::Vector3 transf;

  // find the z-range of the current frustum as seen from the light
  // in order to increase precision

  // note that only the z-component is need and thus
  // the multiplication can be simplified
  // transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] *
  // f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
  transf = f.point[0];
  transf.Transform(nv_mvp);

  minZ = transf[2];
  maxZ = transf[2];
  for (int i = 1; i < 8; i++) {
    transf = f.point[i];
    transf.Transform(nv_mvp);
    if (transf[2] > maxZ) maxZ = transf[2];
    if (transf[2] < minZ) minZ = transf[2];
  }
  // make sure all relevant shadow casters are included
  // note that these here are dummy objects at the edges of our scene
  /*for (int i = 0; i < NUM_OBJECTS; i++) {
  transf = nv_mvp * vec4f(obj_BSphere[i].center, 1.0f);
  if (transf.z + obj_BSphere[i].radius > maxZ)
  maxZ = transf.z + obj_BSphere[i].radius;
  //	if(transf.z - obj_BSphere[i].radius < minZ) minZ = transf.z -
  // obj_BSphere[i].radius;
  }*/

  shad_proj.Orthographic(-1.0, 1.0, -1.0, 1.0, -maxZ, -minZ);
  shad_mvp = shad_proj * shad_modelview;

  // find the extends of the frustum slice as projected in light's homogeneous
  // coordinates
  nv_mvp = shad_mvp;
  for (auto i : f.point) {
    transf = i;
    auto w = transf.Transform(nv_mvp);

    transf[0] /= w;
    transf[1] /= w;

    if (transf[0] > maxX) maxX = transf[0];
    if (transf[0] < minX) minX = transf[0];
    if (transf[1] > maxY) maxY = transf[1];
    if (transf[1] < minY) minY = transf[1];
  }

  float scaleX = 2.0f / (maxX - minX);
  float scaleY = 2.0f / (maxY - minY);
  float offsetX = -0.5f * (maxX + minX) * scaleX;
  float offsetY = -0.5f * (maxY + minY) * scaleY;

  // apply a crop matrix to modify the projection matrix we got from glOrtho.
  nv_mvp.Identity();
  nv_mvp.data[0] = scaleX;
  nv_mvp.data[5] = scaleY;
  nv_mvp.data[3] = offsetX;
  nv_mvp.data[7] = offsetY;
  nv_mvp.Transpose();
  shad_crop = nv_mvp;
  shad_proj *= shad_crop;
  shadow_cascades.push_back(shad_proj);

  return minZ;
}
}  // namespace lib_graphics
