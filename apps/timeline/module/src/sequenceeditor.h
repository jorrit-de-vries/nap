#pragma once

// internal includes
#include "sequence.h"
#include "sequenceplayer.h"

// external includes
#include <nap/resource.h>
#include <parameter.h>

namespace nap
{
	//////////////////////////////////////////////////////////////////////////

	// forward declares
	class SequenceEditorController;

	/**
	 * SequenceEditor
	 * The SequenceEditor is responsible for editing the sequence (model) and makes sure the model stays valid during editing.
	 * It also holds a resource ptr to a player, to make sure that editing the sequence stays thread safe.
	 */
	class NAPAPI SequenceEditor : 
		public Resource
	{
		friend class SequenceEditorGUI;

		RTTI_ENABLE(Resource)
	public:
		virtual bool init(utility::ErrorState& errorState);
	public:
		// properties
		ResourcePtr<SequencePlayer> mSequencePlayer = nullptr; ///< Property: 'Sequence Player' ResourcePtr to the sequence player

		const Sequence& getSequence() const;
	protected:
		std::unique_ptr<SequenceEditorController> mController = nullptr;
	private:
		SequenceEditorController& getController();
	};

	/**
	 * SequenceEditorController 
	 * The actual controller with methods that the view can call
	 */
	class SequenceEditorController
	{
	public:
		/**
		 * Constructor
		 * @param sequencePlayer reference to the sequence player
		 * @param sequence reference to the sequence ( model )
		 */
		SequenceEditorController(SequencePlayer& sequencePlayer, Sequence& sequence) 
			: mSequence(sequence), mSequencePlayer(sequencePlayer) {}

		/**
		 * segmentDurationChange
		 * @param segmentID the id of the segement we need to edit
		 * @param amount the amount that the duration of this segment should change
		 */
		void segmentDurationChange(std::string segmentID, float amount);

		/**
		 * save
		 * saves the sequence
		 */
		void save();

		/**
		 * insertSequence
		 * insert sequence at given time
		 * @param trackID the track that the segment gets inserted to
		 * @param time the time at which the track gets inserted
		 */
		void insertSequence(std::string trackID, double time);
	protected:
		Sequence&			mSequence;
		SequencePlayer&		mSequencePlayer;
	};
}
