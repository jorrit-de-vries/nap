// Local Includes
#include "image.h"
#include "nbitmap.h"
#include "nbitmaputils.h"
#include "ntextureutils.h"

// External Includes
#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::Image)
	RTTI_PROPERTY_FILELINK("ImagePath", &nap::Image::mImagePath, 		nap::rtti::EPropertyMetaData::Required, nap::rtti::EPropertyFileType::Image)
	RTTI_PROPERTY("Compressed",			&nap::Image::mCompressed,		nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{

	// Constructor
	Image::Image(const std::string& imgPath) :
		mImagePath(imgPath)
	{
	}


	bool Image::init(utility::ErrorState& errorState)
	{
		if (!errorState.check(!mImagePath.empty(), "Image path not set for ImageResource %s", mID.c_str()))
			return false;

		// Load pixel data in to bitmap
		if (!mPixmap.initFromFile(mImagePath, errorState))
			return false;

		// Get opengl settings from bitmap
		opengl::Texture2DSettings settings;
		if (!errorState.check(opengl::getSettingsFromBitmap(mPixmap.getBitmap(), mCompressed, settings, errorState), "Unable to determine texture settings from bitmap %s", mImagePath.c_str()))
			return false;

		// Initialize texture from bitmap
		BaseTexture2D::init(settings);

		// Set data from bitmap
		getTexture().setData(mPixmap.getBitmap().getData());

		return true;
	}
}
