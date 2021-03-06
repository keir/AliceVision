// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "aliceVision/numeric/numeric.hpp"
#include "aliceVision/camera/cameraCommon.hpp"
#include "aliceVision/geometry/Pose3.hpp"
#include "aliceVision/stl/hash.hpp"

#include <cereal/cereal.hpp>

#include <vector>

namespace aliceVision {
namespace camera {

/// Basis class for all intrinsic parameters of a camera
/// Store the image size & define all basis optical modelization of a camera
struct IntrinsicBase
{
  unsigned int _w, _h;
  double _initialFocalLengthPix = -1;
  std::string _serialNumber;

  IntrinsicBase(unsigned int w = 0, unsigned int h = 0, const std::string& serialNumber = ""):_w(w), _h(h), _serialNumber(serialNumber)
  {}
  virtual ~IntrinsicBase()
  {}
  
  virtual IntrinsicBase* clone() const = 0;
  virtual void assign(const IntrinsicBase& other) = 0;

  virtual bool isValid() const { return _w != 0 && _h != 0; }
  
  unsigned int w() const {return _w;}
  unsigned int h() const {return _h;}
  const std::string& serialNumber() const {return _serialNumber;}
  double initialFocalLengthPix() const {return _initialFocalLengthPix;}
  
  void setWidth(unsigned int w) { _w = w;}
  void setHeight(unsigned int h) { _h = h;}
  void setSerialNumber(const std::string& serialNumber)
  {
    _serialNumber = serialNumber;
  }
  void setInitialFocalLengthPix(double initialFocalLengthPix)
  {
    _initialFocalLengthPix = initialFocalLengthPix;
  }

  // Operator ==
  bool operator==(const IntrinsicBase& other) const {
    return _w == other._w &&
           _h == other._h &&
           _serialNumber == other._serialNumber &&
           getType() == other.getType() &&
           getParams() == other.getParams();
  }

  /// Projection of a 3D point into the camera plane (Apply pose, disto (if any) and Intrinsics)
  Vec2 project(
    const geometry::Pose3 & pose,
    const Vec3 & pt3D,
    bool applyDistortion = true) const
  {
    const Vec3 X = pose(pt3D); // apply pose
    if (applyDistortion && this->have_disto()) // apply disto & intrinsics
      return this->cam2ima( this->add_disto(X.head<2>()/X(2)) );
    else // apply intrinsics
      return this->cam2ima( X.head<2>()/X(2) );
  }

  /// Compute the residual between the 3D projected point X and an image observation x
  Vec2 residual(const geometry::Pose3 & pose, const Vec3 & X, const Vec2 & x) const
  {
    const Vec2 proj = this->project(pose, X);
    return x - proj;
  }
  
  Mat2X residuals(const geometry::Pose3 & pose, const Mat3X & X, const Mat2X & x) const
  {
    assert(X.cols() == x.cols());
    const std::size_t numPts = x.cols();
    Mat2X residuals = Mat2X::Zero(2, numPts);
    for(std::size_t i = 0; i < numPts; ++i)
    {
      residuals.col(i) = residual(pose, X.col(i), x.col(i));
    }
    return residuals;
  }

  // --
  // Virtual members
  // --

  /// Tell from which type the embed camera is
  virtual EINTRINSIC getType() const = 0;

  /// Data wrapper for non linear optimization (get data)
  virtual std::vector<double> getParams() const = 0;

  /// Data wrapper for non linear optimization (update from data)
  virtual bool updateFromParams(const std::vector<double> & params) = 0;

  /// Get bearing vector of p point (image coord)
  virtual Vec3 operator () (const Vec2& p) const = 0;

  /// Transform a point from the camera plane to the image plane
  virtual Vec2 cam2ima(const Vec2& p) const = 0;

  /// Transform a point from the image plane to the camera plane
  virtual Vec2 ima2cam(const Vec2& p) const = 0;

  /// Does the camera model handle a distortion field?
  virtual bool have_disto() const {return false;}

  /// Add the distortion field to a point (that is in normalized camera frame)
  virtual Vec2 add_disto(const Vec2& p) const = 0;

  /// Remove the distortion to a camera point (that is in normalized camera frame)
  virtual Vec2 remove_disto(const Vec2& p) const  = 0;

  /// Return the un-distorted pixel (with removed distortion)
  virtual Vec2 get_ud_pixel(const Vec2& p) const = 0;

  /// Return the distorted pixel (with added distortion)
  virtual Vec2 get_d_pixel(const Vec2& p) const = 0;

  /// Normalize a given unit pixel error to the camera plane
  virtual double imagePlane_toCameraPlaneError(double value) const = 0;

  /// Return the intrinsic (interior & exterior) as a simplified projective projection
  virtual Mat34 get_projective_equivalent(const geometry::Pose3 & pose) const = 0;

  /// Serialization out
  template <class Archive>
  void save( Archive & ar) const
  {
    ar(cereal::make_nvp("width", _w));
    ar(cereal::make_nvp("height", _h));
    ar(cereal::make_nvp("serialNumber", _serialNumber));
    ar(cereal::make_nvp("initialFocalLengthPix", _initialFocalLengthPix));
  }

  /// Serialization in
  template <class Archive>
  void load( Archive & ar)
  {
    ar(cereal::make_nvp("width", _w));
    ar(cereal::make_nvp("height", _h));
    // compatibility with older versions
    try
    {
      ar(cereal::make_nvp("serialNumber", _serialNumber));
    }
    catch(cereal::Exception& e)
    {
      _serialNumber = "";
    }
    try
    {
      ar(cereal::make_nvp("initialFocalLengthPix", _initialFocalLengthPix));
    }
    catch(cereal::Exception& e)
    {
      _initialFocalLengthPix = -1;
    }
  }

  /// Generate an unique Hash from the camera parameters (used for grouping)
  virtual std::size_t hashValue() const
  {
    size_t seed = 0;
    stl::hash_combine(seed, static_cast<int>(this->getType()));
    stl::hash_combine(seed, _w);
    stl::hash_combine(seed, _h);
    stl::hash_combine(seed, _serialNumber);
    const std::vector<double> params = this->getParams();
    for (size_t i=0; i < params.size(); ++i)
      stl::hash_combine(seed, params[i]);
    return seed;
  }
};

/// Return the angle (degree) between two bearing vector rays
inline double AngleBetweenRays(const Vec3 & ray1, const Vec3 & ray2)
{
  const double mag = ray1.norm() * ray2.norm();
  const double dotAngle = ray1.dot(ray2);
  return radianToDegree(acos(clamp(dotAngle/mag, -1.0 + 1.e-8, 1.0 - 1.e-8)));
}

/// Return the angle (degree) between two bearing vector rays
inline double AngleBetweenRays(
  const geometry::Pose3 & pose1,
  const IntrinsicBase * intrinsic1,
  const geometry::Pose3 & pose2,
  const IntrinsicBase * intrinsic2,
  const Vec2 & x1, const Vec2 & x2)
{
  // x = (u, v, 1.0)  // image coordinates
  // X = R.t() * K.inv() * x + C // Camera world point
  // getting the ray:
  // ray = X - C = R.t() * K.inv() * x
  const Vec3 ray1 = (pose1.rotation().transpose() * intrinsic1->operator()(x1)).normalized();
  const Vec3 ray2 = (pose2.rotation().transpose() * intrinsic2->operator()(x2)).normalized();
  return AngleBetweenRays(ray1, ray2);
}

/// Return the angle (degree) between two poses and a 3D point.
inline double AngleBetweenRays(
  const geometry::Pose3 & pose1,
  const geometry::Pose3 & pose2,
  const Vec3 & pt3D)
{
  const Vec3 ray1 = pt3D - pose1.center();
  const Vec3 ray2 = pt3D - pose2.center();
  return AngleBetweenRays(ray1, ray2);
}

} // namespace camera
} // namespace aliceVision
