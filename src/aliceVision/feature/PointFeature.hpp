// This file is part of the AliceVision project.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "aliceVision/numeric/numeric.hpp"
#include <iostream>
#include <iterator>
#include <fstream>
#include <string>
#include <vector>

namespace aliceVision {
namespace feature {

/**
 * Base class for Point features.
 * Store position of a feature point.
 */
class PointFeature {

  friend std::ostream& operator<<(std::ostream& out, const PointFeature& obj);
  friend std::istream& operator>>(std::istream& in, PointFeature& obj);

public:
  PointFeature(float x=0.0f, float y=0.0f)
   : _coords(x, y) {}

  float x() const { return _coords(0); }
  float y() const { return _coords(1); }
  const Vec2f & coords() const { return _coords;}

  float& x() { return _coords(0); }
  float& y() { return _coords(1); }
  Vec2f& coords() { return _coords;}

  template<class Archive>
  void serialize(Archive & ar)
  {
    ar (_coords(0), _coords(1));
  }

protected:
  Vec2f _coords;  // (x, y)
};

typedef std::vector<PointFeature> PointFeatures;

//with overloaded operators:
inline std::ostream& operator<<(std::ostream& out, const PointFeature& obj)
{
  return out << obj._coords(0) << " " << obj._coords(1);
}

inline std::istream& operator>>(std::istream& in, PointFeature& obj)
{
  return in >> obj._coords(0) >> obj._coords(1);
}

/**
 * Base class for ScaleInvariant Oriented Point features.
 * Add scale and orientation description to basis PointFeature.
 */
class SIOPointFeature : public PointFeature {

  friend std::ostream& operator<<(std::ostream& out, const SIOPointFeature& obj);
  friend std::istream& operator>>(std::istream& in, SIOPointFeature& obj);

public:
  SIOPointFeature(float x=0.0f, float y=0.0f,
                  float scale=0.0f, float orient=0.0f)
    : PointFeature(x,y)
    , _scale(scale)
    , _orientation(orient) {}

  float scale() const { return _scale; }
  float& scale() { return _scale; }
  float orientation() const { return _orientation; }
  float& orientation() { return _orientation; }
  
  /**
    * @brief Return the orientation of the feature as an unit vector.
    * @return a unit vector corresponding to the orientation of the feature.
    */
  Vec2f getOrientationVector() const
  {
    return Vec2f(std::cos(orientation()), std::sin(orientation()));
  }

  /**
    * @brief Return the orientation of the feature as a vector scaled to the the scale of the feature.
    * @return a vector corresponding to the orientation of the feature scaled at the scale of the feature.
    */
  Vec2f getScaledOrientationVector() const
  {
    return scale()*getOrientationVector();
  }
  
  bool operator ==(const SIOPointFeature& b) const {
    return (_scale == b.scale()) &&
           (_orientation == b.orientation()) &&
           (x() == b.x()) && (y() == b.y()) ;
  }

  bool operator !=(const SIOPointFeature& b) const {
    return !((*this)==b);
  }

  template<class Archive>
  void serialize(Archive & ar)
  {
    ar (
      _coords(0), _coords(1),
      _scale,
      _orientation);
  }

protected:
  float _scale;        // In pixels.
  float _orientation;  // In radians.
};

//
inline std::ostream& operator<<(std::ostream& out, const SIOPointFeature& obj)
{
  const PointFeature *pf = static_cast<const PointFeature*>(&obj);
  return out << *pf << " " << obj._scale << " " << obj._orientation;
}

inline std::istream& operator>>(std::istream& in, SIOPointFeature& obj)
{
  PointFeature *pf = static_cast<PointFeature*>(&obj);
  return in >> *pf >> obj._scale >> obj._orientation;
}

/// Read feats from file
template<typename FeaturesT >
inline void loadFeatsFromFile(
  const std::string & sfileNameFeats,
  FeaturesT & vec_feat)
{
  vec_feat.clear();

  std::ifstream fileIn(sfileNameFeats);

  if(!fileIn.is_open())
    throw std::runtime_error("Can't load features file, can't open '" + sfileNameFeats + "' !");
  
  std::copy(
    std::istream_iterator<typename FeaturesT::value_type >(fileIn),
    std::istream_iterator<typename FeaturesT::value_type >(),
    std::back_inserter(vec_feat));
  if(fileIn.bad())
    throw std::runtime_error("Can't load features file, '" + sfileNameFeats + "' is incorrect !");
  fileIn.close();
}

/// Write feats to file
template<typename FeaturesT >
inline void saveFeatsToFile(
  const std::string & sfileNameFeats,
  FeaturesT & vec_feat)
{
  std::ofstream file(sfileNameFeats.c_str());

  if (!file.is_open())
    throw std::runtime_error("Can't save features file, can't open '" + sfileNameFeats + "' !");

  std::copy(vec_feat.begin(), vec_feat.end(), std::ostream_iterator<typename FeaturesT::value_type >(file,"\n"));

  if(!file.good())
    throw std::runtime_error("Can't save features file, '" + sfileNameFeats + "' is incorrect !");

  file.close();
}

/// Export point feature based vector to a matrix [(x,y)'T, (x,y)'T]
template< typename FeaturesT, typename MatT >
void PointsToMat(
  const FeaturesT & vec_feats,
  MatT & m)
{
  m.resize(2, vec_feats.size());
  typedef typename FeaturesT::value_type ValueT; // Container type

  size_t i = 0;
  for( typename FeaturesT::const_iterator iter = vec_feats.begin();
    iter != vec_feats.end(); ++iter, ++i)
  {
    const ValueT & feat = *iter;
    m.col(i) << feat.x(), feat.y();
  }
}

} // namespace feature
} // namespace aliceVision
