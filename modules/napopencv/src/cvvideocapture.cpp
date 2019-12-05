// Local Includes
#include "cvvideocapture.h"

// External Includes
#include <nap/logger.h>

// nap::cvvideocapture run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::CVVideoCapture)
	RTTI_PROPERTY("ConvertRGB",		&nap::CVVideoCapture::mConvertRGB,		nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("FlipHorizontal", &nap::CVVideoCapture::mFlipHorizontal,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("FlipVertical",	&nap::CVVideoCapture::mFlipVertical,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Backend",		&nap::CVVideoCapture::mAPIPreference,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	CVVideoCapture::~CVVideoCapture()			{ }


	bool CVVideoCapture::grab(cv::UMat& target)
	{
		// Check if a new frame is available
		if (!mFrameAvailable)
			return false;

		// Copy last captured frame using a deep copy.
		// Again, the deep copy is necessary because a weak copy allows
		// for the data to be updated by the capture loop whilst still processing on another thread.
		std::lock_guard<std::mutex> lock(mCaptureMutex);
		mCaptureMat.copyTo(target);
		mFrameAvailable = false;
		return true;
	}
	

	void CVVideoCapture::capture()
	{
		{
			std::lock_guard<std::mutex> lock(mCaptureMutex);
			mCaptureFrame = true;
		}
		mCaptureCondition.notify_one();
	}


	bool CVVideoCapture::start(utility::ErrorState& errorState)
	{
		assert(!mCaptureDevice.isOpened());
		if (!onOpen(mCaptureDevice, static_cast<int>(mAPIPreference), errorState))
			return false;

		// Start capture task
		mStopCapturing = false;
		mCaptureTask = std::async(std::launch::async, std::bind(&CVVideoCapture::captureTask, this));
		return true;
	}


	void CVVideoCapture::setProperty(cv::VideoCaptureProperties propID, double value)
	{
		{
			std::lock_guard<std::mutex> lock(mCaptureMutex);
			mProperties[propID] = value;
			mCaptureFrame = true;
		}
		mCaptureCondition.notify_one();
	}


	double CVVideoCapture::getProperty(cv::VideoCaptureProperties propID) const
	{
		return mCaptureDevice.get(propID);
	}


	void CVVideoCapture::stop()
	{
		onClose();

		// Stop capturing thread and notify worker
		{
			std::lock_guard<std::mutex> lock(mCaptureMutex);
			mStopCapturing = true;
		}
		mCaptureCondition.notify_one();

		// Wait till exit
		if (mCaptureTask.valid())
			mCaptureTask.wait();

		// Release capture device
		if (mCaptureDevice.isOpened())
			mCaptureDevice.release();
	}


	int CVVideoCapture::getWidth() const
	{
		return static_cast<int>(getProperty(cv::CAP_PROP_FRAME_WIDTH));
	}


	int CVVideoCapture::getHeight() const
	{
		return static_cast<int>(getProperty(cv::CAP_PROP_FRAME_HEIGHT));
	}


	void CVVideoCapture::captureTask()
	{
		// Wait for playback to be enabled, a new frame request is issued or request to stop is made
		// Exit loop immediately when a stop is requested. Otherwise process next frame
		cv::UMat cap_frame;
		std::unordered_map<int, double> properties;
		bool set_properties = false;

		while(!mStopCapturing)
		{
			{
				std::unique_lock<std::mutex> lock(mCaptureMutex);
				mCaptureCondition.wait(lock, [this]()
				{
					return (mStopCapturing || mCaptureFrame);
				});

				// Exit loop when exit has been triggered
				if (mStopCapturing)
				{
					break;
				}

				// Swap properties
				properties.swap(mProperties);

				// Clear all properties
				mProperties.clear();
				mCaptureFrame = false;
			}

			// Apply properties
			for (const auto& prop : properties)
			{
				//nap::Logger::info("setting property: %d to %d", prop.first, (int)prop.second);
				if (!mCaptureDevice.set(prop.first, prop.second))
					nap::Logger::warn("%s: unable to set property: %s to: %.02f", mID.c_str(), prop.first, prop.second);
			}

			// Fetch frame
			if (!mCaptureDevice.grab())
			{
				nap::Logger::warn("%s: failed to grab frame. Device disconnected or end of stream.", mID.c_str());
				continue;
			}

			nap::utility::ErrorState grab_error;
			if (!onRetrieve(mCaptureDevice, cap_frame, grab_error))
			{
				grab_error.fail("%s: failed to decode frame", mID.c_str());
				nap::Logger::error(grab_error.toString());
				continue;
			}

			// Convert to RGB
			if(mConvertRGB)
				cv::cvtColor(cap_frame, cap_frame, cv::COLOR_BGR2RGB);

			// Flip horizontal
			if (mFlipHorizontal)
				cv::flip(cap_frame, cap_frame, 1);

			// Flip vertical
			if (mFlipVertical)
				cv::flip(cap_frame, cap_frame, 0);

			// Deep copy the captured frame to our storage matrix.
			// This updates the data of our storage container and ensures the same dimensionality.
			// We need to perform a deep-copy because if we choose to use a shallow copy, 
			// by the time the frame is grabbed the data 'mCaptureMat' points to could have changed, 
			// as it references the same data as in 'cap_frame'. And the 'cap_frame' process loop already started.
			// Performing a deep_copy ensures that when the data is grabbed it will contain the latest full processed frame.
			//
			// Alternatively, we could make the 'cap_frame' variable local to this loop, but that creates
			// more overhead than the copy below.
			{
				std::lock_guard<std::mutex> lock(mCaptureMutex);
				cap_frame.copyTo(mCaptureMat);
				onCopy();
			}

			// New frame is available
			mFrameAvailable = true;
		}
	}

}