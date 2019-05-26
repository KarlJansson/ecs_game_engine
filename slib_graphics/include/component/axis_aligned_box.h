#pragma once
#include <cmath>
#include "camera.h"
#include "vector_def.h"

namespace lib_graphics {
class BoundingVolume {
 public:
  BoundingVolume(lib_core::Vector3 center, lib_core::Vector3 extent)
      : center(center), extent(extent) {}
  BoundingVolume(lib_core::Vector3 center, float radius)
      : center(center), extent(radius) {}
  BoundingVolume() = default;

  lib_core::Vector3 center;
  lib_core::Vector3 extent;
};

class AxisAlignedBox {
 public:
  AxisAlignedBox(BoundingVolume base) : data(base) {}

  [[nodiscard]] bool Overlap(BoundingVolume box) const {
    return std::abs(data.center[0] - box.center[0]) <=
               (data.extent[0] + box.extent[0]) &&
           std::abs(data.center[1] - box.center[1]) <=
               (data.extent[1] + box.extent[1]) &&
           std::abs(data.center[2] - box.center[2]) <=
               (data.extent[2] + box.extent[2]);
  }

  [[nodiscard]] bool Contains(BoundingVolume box) const {
    return box.center[0] - box.extent[0] >= data.center[0] - data.extent[0] &&
           box.center[1] - box.extent[1] >= data.center[1] - data.extent[1] &&
           box.center[2] - box.extent[2] >= data.center[2] - data.extent[2] &&
           box.center[0] + box.extent[0] <= data.center[0] + data.extent[0] &&
           box.center[1] + box.extent[1] <= data.center[1] + data.extent[1] &&
           box.center[2] + box.extent[2] <= data.center[2] + data.extent[2];
  }

  [[nodiscard]] bool Overlap(AxisAlignedBox box) const {
    return Overlap(box.data);
  }
  [[nodiscard]] bool Contains(AxisAlignedBox box) const {
    return Contains(box.data);
  }

  BoundingVolume data;
};

class BoundingFrustum {
 public:
  BoundingFrustum(Camera::FrustumPlanes planes) : planes_(planes) {}

  [[nodiscard]] bool Overlap(BoundingVolume volume) const {
    std::array<lib_core::Vector3, 2> min_max;
    min_max[0] = volume.center - volume.extent;
    min_max[1] = volume.center + volume.extent;

    float dp;
    int px, py, pz;

    for (auto p : planes_.planes) {
      px = static_cast<int>(p.normal[0] > 0.0f);
      py = static_cast<int>(p.normal[1] > 0.0f);
      pz = static_cast<int>(p.normal[2] > 0.0f);

      dp = (p.normal[0] * min_max[px][0]) + (p.normal[1] * min_max[py][1]) +
           (p.normal[2] * min_max[pz][2]);

      if (dp < -p.w) return false;
    }

    return true;
  }

  [[nodiscard]] bool Contains(BoundingVolume volume) const {
    std::array<lib_core::Vector3, 2> min_max;
    min_max[0] = volume.center - volume.extent;
    min_max[1] = volume.center + volume.extent;

    float dp;
    for (auto& i : min_max) {
      for (auto p : planes_.planes) {
        dp = (p.normal[0] * i[0]) + (p.normal[1] * i[1]) + (p.normal[2] * i[2]);

        if (dp < -p.w) return false;
      }
    }

    min_max[0][2] += volume.extent[2];
    min_max[1][2] -= volume.extent[2];

    for (auto& i : min_max) {
      for (auto p : planes_.planes) {
        dp = (p.normal[0] * i[0]) + (p.normal[1] * i[1]) + (p.normal[2] * i[2]);

        if (dp < -p.w) return false;
      }
    }

    return true;
  }

  [[nodiscard]] bool Overlap(AxisAlignedBox box) const {
    return Overlap(box.data);
  }
  [[nodiscard]] bool Contains(AxisAlignedBox box) const {
    return Contains(box.data);
  }

  Camera::FrustumPlanes planes_;
};

class BoundingSphere {
 public:
  BoundingSphere(BoundingVolume base) : data(base) {}

  [[nodiscard]] bool Overlap(BoundingSphere sphere) const {
    return Overlap(sphere.data);
  }

  [[nodiscard]] bool Contains(BoundingSphere sphere) const {
    return Contains(sphere.data);
  }

  [[nodiscard]] bool Overlap(BoundingVolume box) const {
    return (box.center - data.center).Length() < box.extent[0] + data.extent[0];
  }

  [[nodiscard]] bool Contains(BoundingVolume box) const {
    return (box.center - data.center).Length() + box.extent[0] < data.extent[0];
  }

  [[nodiscard]] bool Overlap(AxisAlignedBox box) const {
    return std::abs(data.center[0] - box.data.center[0]) <=
               (data.extent[0] + box.data.extent[0]) &&
           std::abs(data.center[1] - box.data.center[1]) <=
               (data.extent[1] + box.data.extent[1]) &&
           std::abs(data.center[2] - box.data.center[2]) <=
               (data.extent[2] + box.data.extent[2]);
  }

  [[nodiscard]] bool Contains(AxisAlignedBox box) const {
    return box.data.center[0] - box.data.extent[0] >=
               data.center[0] - data.extent[0] &&
           box.data.center[1] - box.data.extent[1] >=
               data.center[1] - data.extent[1] &&
           box.data.center[2] - box.data.extent[2] >=
               data.center[2] - data.extent[2] &&
           box.data.center[0] + box.data.extent[0] <=
               data.center[0] + data.extent[0] &&
           box.data.center[1] + box.data.extent[1] <=
               data.center[1] + data.extent[1] &&
           box.data.center[2] + box.data.extent[2] <=
               data.center[2] + data.extent[2];
  }

  BoundingVolume data;
};

}  // namespace lib_graphics
