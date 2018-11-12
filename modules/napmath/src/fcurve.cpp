#include "fcurve.h"

RTTI_BEGIN_ENUM(nap::math::FCurveInterp)
	RTTI_ENUM_VALUE(nap::math::FCurveInterp::Linear,   "Linear"),
	RTTI_ENUM_VALUE(nap::math::FCurveInterp::Stepped,  "Stepped"),
	RTTI_ENUM_VALUE(nap::math::FCurveInterp::Bezier,   "Bezier")
RTTI_END_ENUM

RTTI_BEGIN_STRUCT(nap::math::FloatFComplex)
		RTTI_PROPERTY("Time", &nap::math::FloatFComplex::mTime, nap::rtti::EPropertyMetaData::Default)
		RTTI_PROPERTY("Value", &nap::math::FloatFComplex::mValue, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_STRUCT(nap::math::FloatFCurvePoint)
	RTTI_PROPERTY("Position", &nap::math::FloatFCurvePoint::mPos, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("InTangent", &nap::math::FloatFCurvePoint::mInTan, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("OutTangent", &nap::math::FloatFCurvePoint::mOutTan, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("InterpolationType", &nap::math::FloatFCurvePoint::mInterp, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("AlignedTangents", &nap::math::FloatFCurvePoint::mTangentsAligned, nap::rtti::EPropertyMetaData::Default)
RTTI_END_STRUCT

RTTI_BEGIN_CLASS(nap::math::FloatFCurve)
	RTTI_PROPERTY("Points", &nap::math::FloatFCurve::mPoints, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_DEFINE_BASE(nap::math::FloatFCurvePoint);
RTTI_DEFINE_BASE(nap::math::Vec2FCurvePoint);
RTTI_DEFINE_BASE(nap::math::Vec3FCurvePoint);
RTTI_DEFINE_BASE(nap::math::Vec4FCurvePoint);

#define DEFAULT_TAN_OFFSET 0.1f

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFAULT CURVE CONSTRUCTORS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<>
nap::math::FloatFCurve::FCurve() {
	mPoints.emplace_back(FloatFCurvePoint({0.0f, 0.0f}, {-DEFAULT_TAN_OFFSET, 0.0f}, {DEFAULT_TAN_OFFSET, 0.0f}));
	mPoints.emplace_back(FloatFCurvePoint({1.0f, 1.0f}, {-DEFAULT_TAN_OFFSET, 0.0f}, {DEFAULT_TAN_OFFSET, 0.0f}));
}


template<>
nap::math::Vec2FCurve::FCurve() {
	glm::vec2 nil(0.0f, 0.0f);
	glm::vec2 one(1.0f, 1.0f);
	mPoints.emplace_back(Vec2FCurvePoint({0.0f, nil}, {-DEFAULT_TAN_OFFSET, nil}, {DEFAULT_TAN_OFFSET, nil}));
	mPoints.emplace_back(Vec2FCurvePoint({0.0f, one}, {-DEFAULT_TAN_OFFSET, nil}, {DEFAULT_TAN_OFFSET, nil}));
}