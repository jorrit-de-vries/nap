/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "snapshot.h"
#include "snapshotrendertarget.h"
#include "renderablemeshcomponent.h"

#include <nap/logger.h>
#include <FreeImage.h>
#undef BYTE
#define STITCH_COMBINE 256

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::Snapshot)
	RTTI_CONSTRUCTOR(nap::Core&)
	RTTI_PROPERTY("Width", &nap::Snapshot::mWidth, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Height", &nap::Snapshot::mHeight, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("MaxCellWidth", &nap::Snapshot::mMaxCellWidth, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("MaxCellHeight", &nap::Snapshot::mMaxCellHeight, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("OutputDirectory", &nap::Snapshot::mOutputDirectory, nap::rtti::EPropertyMetaData::FileLink)
	RTTI_PROPERTY("ImageFileFormat", &nap::Snapshot::mImageFileFormat, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("TextureFormat", &nap::Snapshot::mTextureFormat, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("SampleShading", &nap::Snapshot::mSampleShading, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RequestedSamples", &nap::Snapshot::mRequestedSamples, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ClearColor", &nap::Snapshot::mClearColor, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

/**
 * Deduce bitmap type from render texture format. Also informs of the supported output types.
 * @param format the render texture format
 * @return the associated FreeImage type
 */
static FREE_IMAGE_TYPE getFreeImageType(nap::RenderTexture2D::EFormat format)
{
	switch (format)
	{
	case nap::RenderTexture2D::EFormat::R8:
	case nap::RenderTexture2D::EFormat::RGBA8:
	case nap::RenderTexture2D::EFormat::BGRA8:
		return FREE_IMAGE_TYPE::FIT_BITMAP;
	case nap::RenderTexture2D::EFormat::R16:
		return FREE_IMAGE_TYPE::FIT_UINT16;
	case nap::RenderTexture2D::EFormat::RGBA16:
		return FREE_IMAGE_TYPE::FIT_RGBA16;
	}
	return FREE_IMAGE_TYPE::FIT_UNKNOWN;
}

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	//////////////////////////////////////////////////////////////////////////
	// Snapshot
	//////////////////////////////////////////////////////////////////////////

	Snapshot::Snapshot(Core& core) :
		mRenderService(core.getService<RenderService>()) {}


	bool Snapshot::init(utility::ErrorState& errorState)
	{
		assert(mWidth > 0 && mHeight > 0);
		assert(mMaxCellWidth > 0 && mMaxCellHeight > 0);

		// Ensure the RenderTexture2D format is supported for writing
		if (!errorState.check(getFreeImageType(mTextureFormat) != FREE_IMAGE_TYPE::FIT_UNKNOWN, 
			"%s: Unsupported RenderTexture2D format (%s) for writing to disk", mID.c_str(), rtti::Variant(mTextureFormat).to_string().c_str()))
			return false;

		// Make sure not to create textures that exceed the hardware image dimension limit
		uint32 max_image_dimension = mRenderService->getPhysicalDeviceProperties().limits.maxImageDimension2D;
		mMaxCellWidth = std::min(mMaxCellWidth, max_image_dimension);
		mMaxCellHeight = std::min(mMaxCellHeight, max_image_dimension);

		// Subdivide into cells if the max cellwidth|cellheight are smaller than the snapshot size
		mNumRows = ceil(mWidth / static_cast<double>(mMaxCellWidth));
		mNumColumns = ceil(mHeight / static_cast<double>(mMaxCellHeight));

		assert(mNumRows > 0 && mNumColumns > 0);
		assert(mNumRows < mWidth / 2 && mNumColumns < mHeight / 2);

		mNumCells = mNumRows * mNumColumns;
		mCellSize = { mWidth / mNumRows, mHeight / mNumColumns };

		// Inform user in case we have to subdivide the texture
		if (mNumCells > 1)
		{
			Logger::info("%s: Dividing target buffer into %d %dx%d cells", mID.c_str(), mNumRows*mNumColumns, mCellSize.x, mCellSize.y);
		}

		// Check if we need to render to BGRA for image formats on little endian machines
		// We can ignore this check when rendering 16bit color
		if (mTextureFormat == RenderTexture2D::EFormat::RGBA8 || mTextureFormat == RenderTexture2D::EFormat::BGRA8)
		{
			bool is_little_endian = FI_RGBA_RED == 2;
			mTextureFormat = is_little_endian ? RenderTexture2D::EFormat::BGRA8 : RenderTexture2D::EFormat::RGBA8;
		}

		// Create textures
		mColorTextures.resize(mNumCells);
		for (auto& cell : mColorTextures)
		{
			cell = std::make_unique<RenderTexture2D>(mRenderService->getCore());
			cell->mWidth = mCellSize.x;
			cell->mHeight = mCellSize.y;
			cell->mFill = false;
			cell->mUsage = ETextureUsage::DynamicRead;
			cell->mFormat = mTextureFormat;

			if (!cell->init(errorState)) 
			{
				errorState.fail("%s: Failed to initialize snapshot cell textures", mID.c_str());
				return false;
			}
		}

		// Keep a record of updated bitmaps
		mCellUpdateFlags.resize(mNumCells, false);

		// Create render target
		mRenderTarget = std::make_unique<SnapshotRenderTarget>(mRenderService->getCore());
		if (!mRenderTarget->init(this, errorState)) 
		{
			errorState.fail("%s: Failed to initialize snapshot rendertarget", mID.c_str());
			return false;
		}

		// Write the destination bitmap when onBitmapsUpdated is triggered
		onCellsUpdated.connect(mSaveBitmapSlot);

		return true;
	}


	void Snapshot::setClearColor(const glm::vec4& color)
	{
		mRenderTarget->setClearColor(color);
	}


	bool Snapshot::snap(PerspCameraComponentInstance& camera, std::vector<RenderableComponentInstance*>& comps)
	{
		camera.setGridDimensions(mNumRows, mNumColumns);

		for (int i = 0; i < mNumCells; i++) 
		{
			uint32 x = i % mNumColumns;
			uint32 y = i / mNumRows;
			camera.setGridLocation(y, x);

			mRenderTarget->setCellIndex(i);
			mRenderTarget->beginRendering();

			mRenderService->renderObjects(*mRenderTarget, camera, comps);
			mRenderTarget->endRendering();
		}
		camera.setGridLocation(0, 0);
		camera.setGridDimensions(1, 1);

		// Create a surface descriptor for the fullsize bitmap
		SurfaceDescriptor fullsize_surface_descriptor = mColorTextures[0]->getDescriptor();
		fullsize_surface_descriptor.mWidth = mWidth;
		fullsize_surface_descriptor.mHeight = mHeight;

		// Allocate the full-size bitmap file buffer if it is empty
		if (!mDestBitmapFileBuffer)
		{
			mDestBitmapFileBuffer = std::make_unique<BitmapFileBuffer>(fullsize_surface_descriptor);
		}

		// Get bitmap storage and type info
		FREE_IMAGE_TYPE fi_image_type = getFreeImageType(mTextureFormat);
		int bits_per_pixel = mColorTextures[0]->getDescriptor().getBytesPerPixel() * 8;

		// Insert callbacks for copying image data per cell from staging buffer directly into the destination bitmap
		for (int i = 0; i < mNumCells; i++) 
		{
			mColorTextures[i]->asyncGetData([this, index = i, bpp = bits_per_pixel, type = fi_image_type](const void* data, size_t bytes)
			{
				// Wrap staging buffer data in a bitmap header
				int cell_pitch = mCellSize.x*(bpp/8);
				FIBITMAP* fi_bitmap_src = FreeImage_ConvertFromRawBitsEx(
					false, (uint8*)data, type, mCellSize.x, mCellSize.y, cell_pitch, bpp, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK
				);

				// Unflatten index
				uint32 x = index % mNumColumns;
				uint32 y = index / mNumRows;

				// Calculate area to copy
				glm::u32vec2 min = { x*mCellSize.x, y*mCellSize.y };
				glm::u32vec2 max = min + mCellSize;

				// Cast to a FreeImage bitmap handle
				FIBITMAP* fi_bitmap_handle = reinterpret_cast<FIBITMAP*>(mDestBitmapFileBuffer->getHandle());

				// Create view into destination bitmap
				FIBITMAP* fi_bitmap_dst = FreeImage_CreateView(fi_bitmap_handle, min.x, min.y, max.x, max.y);

				// Copy bitmap header src into dest
				FreeImage_Paste(fi_bitmap_dst, fi_bitmap_src, 0, 0, STITCH_COMBINE);

				// Unload headers
				FreeImage_Unload(fi_bitmap_src);
				FreeImage_Unload(fi_bitmap_dst);

				// Keep a record of updated bitmaps
				mCellUpdateFlags[index] = true;

				if (mNumCells == 1 || std::find(std::begin(mCellUpdateFlags), std::end(mCellUpdateFlags), false) == std::end(mCellUpdateFlags)) 
				{
					onCellsUpdated();
					mCellUpdateFlags.assign(mNumCells, false);
				}
			});
		}
		onSnapshot();
		return true;
	}


	bool Snapshot::save()
	{
		// Save to snapshots directory in executable directory if the given output directory is empty
		std::string output_dir = mOutputDirectory.empty() ? utility::joinPath({ utility::getExecutableDir(), "snapshots" }) : mOutputDirectory;
		std::string path = utility::appendFileExtension(utility::joinPath(
		{
			output_dir.c_str(),
			timeFormat(getCurrentTime(), "%Y%m%d_%H%M%S_%ms").c_str()
		}), utility::toLower(rtti::Variant(mImageFileFormat).to_string()));

		utility::ErrorState error_state;
		if (!mDestBitmapFileBuffer->save(path, error_state))
		{
			error_state.fail("%s: Failed to save snapshot to %s", mID.c_str(), path.c_str());
			nap::Logger::error("%s", error_state.toString().c_str());
			return false;
		};

		if (!error_state.hasErrors())
		{
			onSnapshotSaved(path);
		}
		return true;
	}
}
