#pragma once

// internal includes
#include "sequence.h"
#include "sequenceplayer.h"
#include "sequenceeditorview.h"

// external includes
#include <nap/resource.h>
#include <parameter.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	/**
	 */
	class NAPAPI SequenceEditor : public Resource
	{
		friend class SequenceEditorGUI;

		RTTI_ENABLE(Resource)
	public:
		virtual bool init(utility::ErrorState& errorState);

		void registerView(SequenceEditorView* sequenceEditorView);

		void unregisterView(SequenceEditorView* sequenceEditorView);
	public:
		ResourcePtr<SequencePlayer> mSequencePlayer = nullptr;
	protected:
		// SequenceEditorGUI interface
		const Sequence& getSequence();

	protected:
		std::list<SequenceEditorView*> mViews;

		// slots
		Slot<const SequenceTrack&, const SequenceTrackSegment&, float> mSegmentDurationChangeSlot;
		void segmentDurationChange(const SequenceTrack& track, const SequenceTrackSegment& segment, float amount);
	
		Slot<> mSaveSlot;
		void save();
	};
}
