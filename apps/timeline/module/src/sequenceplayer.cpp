// local includes
#include "sequenceplayer.h"
#include "sequenceutils.h"
#include "sequencetracksegmentcurve.h"

// nap include
#include <nap/logger.h>
#include <parametervec.h>

// external includes
#include <utility/fileutils.h>
#include <rtti/jsonwriter.h>
#include <rtti/jsonreader.h>
#include <rtti/defaultlinkresolver.h>
#include <fstream>

RTTI_BEGIN_CLASS(nap::SequencePlayer)
RTTI_PROPERTY("Default Show", &nap::SequencePlayer::mDefaultShow, nap::rtti::EPropertyMetaData::FileLink)
RTTI_PROPERTY("Linked Parameters", &nap::SequencePlayer::mParameters, nap::rtti::EPropertyMetaData::Default)
RTTI_PROPERTY("Frequency", &nap::SequencePlayer::mFrequency, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	bool SequencePlayer::init(utility::ErrorState& errorState)
	{
		if (!Resource::init(errorState))
		{
			return false;
		}

		if (!mCreateDefaultShowOnFailure)
		{
			if (errorState.check(load(mDefaultShow, errorState), "Error loading default sequence"))
			{
				return false;
			}
		}
		else if (!load(mDefaultShow, errorState))
		{
			nap::Logger::info(*this, errorState.toString());
			nap::Logger::info(*this, "Error loading default show, creating default sequence based on given parameters");
		
			std::unordered_set<std::string> objectIDs;
			mSequence = sequenceutils::createDefaultSequence(mParameters, mReadObjects, objectIDs);

			nap::Logger::info(*this, "Done creating default sequence, saving it");
			if (errorState.check(!save(mDefaultShow, errorState), "Error saving sequence"))
			{
				return false;
			}
		}

		// launch player thread
		mUpdateThreadRunning = true;
		mUpdateTask = std::async(std::launch::async, std::bind(&SequencePlayer::onUpdate, this));

		return true;
	}


	void SequencePlayer::onDestroy()
	{
		// stop running thread
		mUpdateThreadRunning = false;
		if (mUpdateTask.valid())
		{
			mUpdateTask.wait();
		}
	}


	void SequencePlayer::play()
	{
		std::unique_lock<std::mutex> l = lock();
		mIsPlaying = true;
		mIsPaused = false;
	}


	void SequencePlayer::pause()
	{
		std::unique_lock<std::mutex> l = lock();
		mIsPaused = true;
	}


	void SequencePlayer::stop()
	{
		std::unique_lock<std::mutex> l = lock();
		mIsPlaying = false;
		mIsPaused = false;
	}


	bool SequencePlayer::save(const std::string& name, utility::ErrorState& errorState)
	{
		std::unique_lock<std::mutex> l = lock();

		// Ensure the presets directory exists
		const std::string dir = "sequences";
		utility::makeDirs(utility::getAbsolutePath(dir));

		std::string show_path = name;

		// Serialize current set of parameters to json
		rtti::JSONWriter writer;
		if (!rtti::serializeObjects(rtti::ObjectList{ mSequence }, writer, errorState))
			return false;

		// Open output file
		std::ofstream output(show_path, std::ios::binary | std::ios::out);
		if (!errorState.check(output.is_open() && output.good(), "Failed to open %s for writing", show_path.c_str()))
			return false;

		// Write to disk
		std::string json = writer.GetJSON();
		output.write(json.data(), json.size());

		return true;
	}


	bool SequencePlayer::load(const std::string& name, utility::ErrorState& errorState)
	{
		std::unique_lock<std::mutex> l = lock();

		//
		rtti::DeserializeResult result;

		//
		std::string timelineName = utility::getFileNameWithoutExtension(name);

		// 
		rtti::Factory factory;
		if (!rtti::readJSONFile(
			name,
			rtti::EPropertyValidationMode::DisallowMissingProperties,
			rtti::EPointerPropertyMode::NoRawPointers,
			factory,
			result,
			errorState))
			return false;

		// Resolve links
		if (!rtti::DefaultLinkResolver::sResolveLinks(result.mReadObjects, result.mUnresolvedPointers, errorState))
			return false;

		// Move ownership of read objects
		mReadObjects.clear();
		mReadObjectIDs.clear();
		for (auto& readObject : result.mReadObjects)
		{
			//
			if (readObject->get_type().is_derived_from<Sequence>())
			{
				mSequence = dynamic_cast<Sequence*>(readObject.get());
			}

			mReadObjectIDs.emplace(readObject->mID);
			mReadObjects.emplace_back(std::move(readObject));
		}

		// init objects
		for (auto& objectPtr : mReadObjects)
		{
			if (!objectPtr->init(errorState))
				return false;
		}

		// check if we have deserialized a sequence
		if (errorState.check(mSequence == nullptr, "sequence is null"))
		{
			return false;
		}

		// create processors
		mProcessors.clear();
		for (auto& track : mSequence->mTracks)
		{
			createProcessor(track->mAssignedParameterID, track->mID);
		}

		mDefaultShow = name;

		return true;
	}


	Sequence& SequencePlayer::getSequence()
	{
		return *mSequence;
	}


	double SequencePlayer::getDuration() const
	{
		return mSequence->mDuration;
	}


	void SequencePlayer::setPlayerTime(double time)
	{
		std::unique_lock<std::mutex> l = lock();

		mTime = time;
		mTime = math::clamp<double>(mTime, 0.0, mSequence->mDuration);
	}

	void SequencePlayer::setPlaybackSpeed(float speed)
	{
		std::unique_lock<std::mutex> l = lock();

		mSpeed = speed;
	}


	double SequencePlayer::getPlayerTime() const
	{
		return mTime;
	}


	bool SequencePlayer::getIsPlaying() const
	{
		return mIsPlaying;
	}


	bool SequencePlayer::getIsPaused() const
	{
		return mIsPaused;
	}


	void SequencePlayer::setIsLooping(bool isLooping)
	{
		std::unique_lock<std::mutex> l = lock();

		mIsLooping = isLooping;
	}

	
	bool SequencePlayer::getIsLooping() const
	{
		return mIsLooping;
	}


	float SequencePlayer::getPlaybackSpeed() const
	{
		return mSpeed;
	}

	
	void SequencePlayer::onUpdate()
	{
		// Compute sleep time in microseconds 
		float sleep_time_microf = 000.0f / static_cast<float>(mFrequency);
		long  sleep_time_micro = static_cast<long>(sleep_time_microf * 1000.0f);

		while (mUpdateThreadRunning)
		{
			// stack push for lock
			{
				// lock
				std::unique_lock<std::mutex> l = lock();

				// advance time
				std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
				std::chrono::nanoseconds elapsed = now - mBefore;
				float deltaTime = std::chrono::duration<float, std::milli>(elapsed).count() / 1000.0f;
				mBefore = now;

				//
				if (mIsPlaying)
				{
					if (!mIsPaused)
					{
						mTime += deltaTime * mSpeed;

						if (mIsLooping)
						{
							if (mTime < 0.0)
							{
								mTime = mSequence->mDuration + mTime;
							}
							else if (mTime > mSequence->mDuration)
							{
								mTime = fmod(mTime, mSequence->mDuration);
							}
						}
						else
						{
							mTime = math::clamp<double>(mTime, 0.0, mSequence->mDuration);
						}
					}

					for (auto& processor : mProcessors)
					{
						processor.second->process(mTime);
					}
				}
			}
			
			std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_micro));
		}
	}


	bool SequencePlayer::createProcessor(
		const std::string& parameterID, 
		const std::string& trackID)
	{
		Parameter* parameter = nullptr;
		for (auto& ownedParameter : mParameters)
		{
			if (ownedParameter->mID == parameterID)
			{
				parameter = ownedParameter.get();
				break;
			}
		}

		if (mProcessors.find(trackID) != mProcessors.end())
		{
			mProcessors.erase(trackID);
		}

		// don't assign anything because we assign an empty parameter
		if (parameterID == "")
		{
			return true;
		}

		if (parameter == nullptr)
		{
			nap::Logger::error(*this, "Couldn't find parameter with id : %s", parameterID.c_str());
			return false;
		}

		for (auto& track : mSequence->mTracks)
		{
			if (track->mID == trackID)
			{
				switch (track->mTrackType)
				{
				case SequenceTrackTypes::FLOAT:
				{
					if (parameter->get_type().is_derived_from<ParameterFloat>())
					{
						ParameterFloat& target = static_cast<ParameterFloat&>(*parameter);

						auto processor = std::make_unique<ProcessorCurve<float, ParameterFloat, float>>(*track.get(), target);
						mProcessors.emplace( trackID, std::move(processor));
					}
					else if (parameter->get_type().is_derived_from<ParameterDouble>())
					{
						ParameterDouble& target = static_cast<ParameterDouble&>(*parameter);

						auto processor = std::make_unique<ProcessorCurve<float, ParameterDouble, double>>(*track.get(), target);
						mProcessors.emplace(trackID, std::move(processor));
					}
					else if (parameter->get_type().is_derived_from<ParameterInt>())
					{
						ParameterInt& target = static_cast<ParameterInt&>(*parameter);

						auto processor = std::make_unique<ProcessorCurve<float, ParameterInt, int>>(*track.get(), target);
						mProcessors.emplace(trackID, std::move(processor));
					}
					else if (parameter->get_type().is_derived_from<ParameterLong>())
					{
						ParameterLong& target = static_cast<ParameterLong&>(*parameter);

						auto processor = std::make_unique<ProcessorCurve<float, ParameterLong, int64_t>>(*track.get(), target);
						mProcessors.emplace(trackID, std::move(processor));
					}
					else
					{
						nap::Logger::error(*this, "Parameter with id %s is not derived from a valid type", parameterID.c_str());
						return false;
					}
				}
					break;
				case SequenceTrackTypes::VEC3:
				{
					if (parameter->get_type().is_derived_from<ParameterVec3>())
					{
						ParameterVec3& target = static_cast<ParameterVec3&>(*parameter);

						auto processor = std::make_unique<ProcessorCurve<glm::vec3, ParameterVec3, glm::vec3>>(*track.get(), target);
						mProcessors.emplace(trackID, std::move(processor));
					}

					else
					{
						nap::Logger::error(*this, "Parameter with id %s is not derived from a valid type", parameterID.c_str());
						return false;
					}
				}
					break;
				}

				break;
			}
		}

		return true;
	}


	std::unique_lock<std::mutex> SequencePlayer::lock()
	{
		return std::unique_lock<std::mutex>(mLock);
	}
}
