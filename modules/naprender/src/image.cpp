#include "image.h"

namespace nap
{
	void Image::update()
	{
		assert(!mBitmap.empty());
		update(mBitmap); 
	}


	Bitmap& Image::getData()
	{
		getData(mBitmap);
		return mBitmap;
	}


	void Image::startGetData()
	{
		getTexture().asyncStartGetData();
	}


	Bitmap& Image::endGetData()
	{
		endGetData(mBitmap);
		return mBitmap;
	}
}
