#pragma once

// External Includes
#include <nap/device.h>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <nap/numeric.h>
#include <thread>
#include <future>
#include <atomic>
#include <queue>

namespace nap
{
	/**
	 * Configurable camera capture parameters.
	 */
	struct NAPAPI CVCameraParameters
	{
		bool	mAutoExposure	= true;		///< Property: 'AutoExposure' if auto exposure is turned on or off
		float	mBrightness		= 1.0f;		///< Property: 'Brightness' camera brightness
		float	mContrast		= 1.0f;		///< Property: 'Contrast' camera contrast
		float	mSaturation		= 1.0f;		///< Property: 'Saturation' camera saturation
		float	mGain			= 1.0f;		///< Property: 'Gain' camera gain
		float	mExposure		= 1.0f;		///< Property: 'Exposure' camera exposure

		/**
		 * @return the parameter properties as a human readable string.
		 */
		std::string toString() const;
	};


	/**
	 * Captures a video stream from a web cam or other peripheral video capture device. 
	 * The captured video frame is stored on the GPU when hardware acceleration is available (OpenCL).
	 * Otherwise the captured video frame is stored on the CPU.
	 * This device captures the video stream on a background thread, call grab() to grab the last recorded video frame.
	 * Camera settings can be provided on startup by enabling 'ApplyParameters'.
	 * After startup the camera settings reflect the current state of the hardware.
	 */
	class NAPAPI CVCamera : public Device
	{
		RTTI_ENABLE(Device)
	public:

		// Stops the device
		virtual ~CVCamera() override;

		/**
		* Initialize this object after de-serialization
		* @param errorState contains the error message when initialization fails
		*/
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * Starts the device
		 * @param errorState contains the error if the device can't be started
		 * @return if the device started
		 */
		virtual bool start(utility::ErrorState& errorState) override;

		/**
		 * Stops the device
		 */
		virtual void stop() override;

		/**
		 * Grabs the last captured frame if new. The result is stored in the given target.
		 * Internally the frame is removed from the queue. 
		 * If there is no new capture the target is not updated and the function returns false.
		 * This operation hands over ownership of the captured frame, data is not copied.
		 * This call is thread safe and can be called every frame. 
		 * @param target holds the last captured frame if new.
		 * @return if the target is updated with the contents of a new captured frame.
		 */
		bool grab(cv::UMat& target);

		/**
		 * Set an OpenCV camera property.
		 * @param propID the property to set
		 * @param value the new property value
		 * @return if the property is applied
		 */
		bool setProperty(cv::VideoCaptureProperties propID, double value);

		/**
		 * Get an OpenCV camera property
		 * @param propID the property to get
		 * @return property value, if available
		 */
		double getProperty(cv::VideoCaptureProperties propID) const;

		/**
		 * Sets and immediately applies new camera parameters.
		 * @param parameters the parameters to set and apply
		 * @param error contains the error message if the operation fails
		 * @return if the operation succeeded
		 */
		bool setParameters(const nap::CVCameraParameters& parameters, utility::ErrorState& error);

		/**
		 * Returns the current camera parameters.
		 * To ensure the parameters are up to date call updateParameters() first.
		 * @return the currently used camera parameters
		 */
		const CVCameraParameters& getParameters()	const			{ return mCameraParameters; }

		/**
		 * Sets the camera parameter values to the settings currently in use by the hardware.
		 * Note that the updated parameters might not be accurate.
		 * The result of this operation greatly depends on the underlying API, OS and hardware itself.
		 */
		void updateParameters();

		/**
		 * Shows the device video capture dialog, only supported by DSHOW backend currently.
		 * @return if call succeeded.
		 */
		bool showSetttingsDialog();

		bool				mConvertRGB = true;			///< Property: 'ConvertRGB' if the frame is converted into RGB
		bool				mFlipHorizontal = false;	///< Property: 'FlipHorizontal' flips the frame on the x-axis
		bool				mFlipVertical = false;		///< Property: 'FlipVertical' flips the frame on the y-axis
		bool				mApplyParameters = false;	///< Property: 'ApplyParameters' If the camera parameters are applied on startup
		nap::uint			mDeviceIndex = 0;			///< Property: 'DeviceIndex' Capture device index
		nap::uint			mFrameWidth	= 640;			///< Property: 'FrameWidth' width of the frame in pixels
		nap::uint			mFrameHeight = 480;			///< Property: 'FrameHeight' height of the frame in pixels
		nap::uint			mAPIPreference = 0;			///< Property: 'API' the capture api preference, 0 = default. See cv::VideoCaptureAPIs for a full list of options.
		CVCameraParameters	mCameraParameters;			///< Property: 'Parameters' all configurable camera parameters

	private:
		cv::VideoCapture	mCaptureDevice;			///< The open-cv video capture device
		cv::UMat			mCaptureMat;			///< The GPU / CPU matrix that holds the most recent captured video frame
		bool				mNewFrame =  false ;	///< If a new frame is captured

		std::future<void>	mCaptureTask;			///< The thread that monitor the read thread
		std::mutex			mCaptureMutex;			///< The mutex that safe guards the capture thread
		bool				mStopCapturing = false;	///< Signals the capture thread to stop capturing video

		/**
		 * Captures new frames.
		 */
		void capture();

		/**
		 * Applies all currently stored camera parameters.
		 * @param error contains the error if operation fails.
		 * @return if the operation succeeded
		 */
		bool applyParameters(utility::ErrorState& error);

		// Queue of captured frames
		std::queue<cv::Mat> mFrameQueue;
	};
}
