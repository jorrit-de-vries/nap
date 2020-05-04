#pragma once

// internal includes
#include "sequencetracksegmentcurve.h"

// external includes
#include <nap/resource.h>
#include <nap/resourceptr.h>
#include <glm/glm.hpp>

namespace nap
{
	/**
	 * Holds a collection of track segments
	 */
	class NAPAPI SequenceTrack : public Resource
	{
		RTTI_ENABLE(Resource)

	public:

		std::string mAssignedObjectIDs;								///< Property: 'Assigned Object ID' Assigned object to this track id
		std::vector<ResourcePtr<SequenceTrackSegment>>	mSegments;	///< Property: 'Segments' Vector holding track segments
	};
}
