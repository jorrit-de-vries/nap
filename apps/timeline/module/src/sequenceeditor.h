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
	 * 
	 */
	enum TanPointTypes
	{
		IN,
		OUT
	};

	/**
	 * 
	 */
	enum SegmentValueTypes
	{
		BEGIN,
		END
	};

	/**
	 * SequenceEditorController 
	 * The actual controller with methods that a view can call
	 */
	class SequenceEditorController
	{
	public:
		/**
		 * Constructor
		 * @param sequencePlayer reference to the sequence player
		 * @param sequence reference to the sequence ( model )
		 */
		SequenceEditorController(SequencePlayer& sequencePlayer) 
			: mSequencePlayer(sequencePlayer) {}

		/**
		 * segmentDurationChange
		 * @param segmentID the id of the segement we need to edit
		 * @param amount the amount that the duration of this segment should change
		 */
		void segmentDurationChange(
			const std::string& segmentID, 
			float amount);

		/**
		 * save
		 * saves the sequence
		 */
		void save();

		/**
		 * insertSegment
		 * insert segment at given time
		 * @param trackID the track that the segment gets inserted to
		 * @param time the time at which the track gets inserted
		 */
		void insertSegment(
			const std::string& trackID, 
			double time);

		/**
		 * deleteSegment
		 * delete segment 
		 * @param trackID the track in which the segment gets deleted 
		 * @param segmentID the segment ID that needs to be deleted
		 */
		void deleteSegment(
			const std::string& trackID,
			const std::string& segmentID);

		/**
		 * changeSegmentEndVlaue
		 * changes the end value of this segment and updates the track accordingly
		 * @param trackID the track in which the segment gets updated
		 * @param segmentID the segment ID that needs to be updated
		 * @param amount the amount that the end value needs to change
		 * @param type the type of value that needs to change ( first or last value )
		 */
		void changeSegmentValue(
			const std::string& trackID, 
			const std::string& segmentID,
			float amount,
			SegmentValueTypes type);

		/**
		 * 
		 */
		void insertCurvePoint(
			const std::string& trackID,
			const std::string& segmentID, 
			float pos);

		/**
		 * 
		 */
		void changeCurvePoint(
			const std::string& trackID, 
			const std::string& segmentID, 
			const int index, 
			float time, float 
			value);

		/**
		 *
		 */
		void deleteCurvePoint(
			const std::string& trackID,
			const std::string& segmentID,
			const int index);

		/**
		 *
		 */
		void changeTanPoint(
			const std::string& trackID,
			const std::string& segmentID,
			const int index,
			TanPointTypes tanType,
			float time, 
			float value);

		void assignNewParameterID(
			const std::string& trackID,
			const std::string& parameterID);

		void addNewTrack();

		void deleteTrack(const std::string& deleteTrackID);

		/**
		 * 
		 */
		SequencePlayer& getSequencePlayer() const;

		const Sequence& getSequence() const;
	protected:
		void updateSegments(const std::unique_lock<std::mutex>& lock);

		SequenceTrackSegment* findSegment(const std::string& trackID, const std::string& segmentID);
	
		SequenceTrack* findTrack(const std::string& trackID);

		void deleteObjectFromSequencePlayer(const std::string& id);
	protected:
		SequencePlayer&		mSequencePlayer;
	};
}
