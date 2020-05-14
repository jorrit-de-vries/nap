#include "sequencecontrollerevent.h"
#include "sequenceutils.h"
#include "sequencetrackevent.h"
#include "sequenceeditor.h"

namespace nap
{
	static bool sRegistered = SequenceController::registerControllerFactory(RTTI_OF(SequenceControllerEvent), [](SequencePlayer& player)->std::unique_ptr<SequenceController> { return std::make_unique<SequenceControllerEvent>(player); });


	static bool sRegisterControllerType = SequenceEditor::registerControllerForTrackType(RTTI_OF(SequenceTrackEvent), RTTI_OF(SequenceControllerEvent));


	void SequenceControllerEvent::segmentEventStartTimeChange(const std::string& trackID, const std::string& segmentID, float amount)
	{
		// pause player thread
		std::unique_lock<std::mutex> l = lock();

		auto* segment = findSegment(trackID, segmentID);
		assert(segment != nullptr); // segment not found

		if (segment != nullptr)
		{
			assert(segment->get_type().is_derived_from(RTTI_OF(SequenceTrackSegmentEventBase))); // type mismatch

			if (segment->get_type().is_derived_from(RTTI_OF(SequenceTrackSegmentEventBase)))
			{
				auto& segment_event = static_cast<SequenceTrackSegmentEventBase&>(*segment);
				segment_event.mStartTime += amount;
			}

		}

		updateTracks();
	}


	void SequenceControllerEvent::insertSegment(const std::string& trackID, double time)
	{
		nap::Logger::warn("insertSegment not used, use insertEventSegment instead");
	}


	void SequenceControllerEvent::deleteSegment(const std::string& trackID, const std::string& segmentID)
	{
		// pause player thread
		std::unique_lock<std::mutex> l = lock();

		//
		Sequence& sequence = getSequence();

		//
		auto* track = findTrack(trackID);
		assert(track != nullptr); // track not found

		if (track != nullptr)
		{
			int segment_index = 0;
			for (auto& segment : track->mSegments)
			{
				if (segment->mID == segmentID)
				{
					// erase it from the list
					track->mSegments.erase(track->mSegments.begin() + segment_index);

					deleteObjectFromSequencePlayer(segmentID);

					break;
				}

				updateTracks();
				segment_index++;
			}
		}
	}


	void SequenceControllerEvent::addNewEventTrack()
	{
		std::unique_lock<std::mutex> l = lock();

		// create sequence track
		std::unique_ptr<SequenceTrackEvent> sequence_track = std::make_unique<SequenceTrackEvent>();
		sequence_track->mID = sequenceutils::generateUniqueID(getPlayerReadObjectIDs());

		//
		getSequence().mTracks.emplace_back(ResourcePtr<SequenceTrackEvent>(sequence_track.get()));

		// move ownership of unique ptrs
		getPlayerOwnedObjects().emplace_back(std::move(sequence_track));
	}


	void SequenceControllerEvent::insertTrack(rttr::type type)
	{
		addNewEventTrack();
	}
}