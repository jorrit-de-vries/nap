#pragma once

// external includes
#include <nap/resource.h>
#include <nap/resourceptr.h>
#include <fcurve.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	/**
	 */
	class NAPAPI SequenceTrackSegment : public Resource
	{
		RTTI_ENABLE(Resource)
	public:
		virtual bool init(utility::ErrorState& errorState) override;
	public:
		double										mStartTime = 0.0;
		double										mDuration = 1.0;

		ResourcePtr<math::FCurve<float, float>>		mCurve;
		float										mStartValue;
		float										mEndValue;
	};
}
