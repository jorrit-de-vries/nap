#pragma once

#include "materialcommon.h"
#include "mesh.h"

namespace nap
{
	class Shader;

	struct PipelineKey
	{
		PipelineKey(const Shader& shader, EDrawMode drawMode, EDepthMode depthMode, EBlendMode blendMode, ECullWindingOrder cullWindingOrder, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount) :
			mShader(&shader),
			mDrawMode(drawMode),
			mDepthMode(depthMode),
			mBlendMode(blendMode),
			mCullWindingOrder(cullWindingOrder),
			mColorFormat(colorFormat),
			mDepthFormat(depthFormat),
			mSampleCount(sampleCount)
		{
		}

		bool operator==(const PipelineKey& rhs) const
		{
			return	
				mShader		== rhs.mShader	&& 
				mDrawMode	== rhs.mDrawMode && 
				mDepthMode == rhs.mDepthMode && 
				mBlendMode == rhs.mBlendMode && 
				mCullWindingOrder == rhs.mCullWindingOrder && 
				mColorFormat == rhs.mColorFormat && 
				mDepthFormat == rhs.mDepthFormat &&
				mSampleCount == rhs.mSampleCount;
		}

		const Shader*			mShader = nullptr;
		EDrawMode				mDrawMode = EDrawMode::TRIANGLES;
		EDepthMode				mDepthMode = EDepthMode::NotSet;
		EBlendMode				mBlendMode = EBlendMode::NotSet;
		ECullWindingOrder		mCullWindingOrder = ECullWindingOrder::Clockwise;
		VkFormat				mColorFormat;
		VkFormat				mDepthFormat;
		VkSampleCountFlagBits	mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	};
}

namespace std
{
	template<>
	struct hash<nap::PipelineKey>
	{
		size_t operator()(const nap::PipelineKey& key) const
		{
			size_t shader_hash = hash<size_t>{}((size_t)key.mShader);
			size_t draw_mode_hash = hash<size_t>{}((size_t)key.mDrawMode);
			size_t depth_mode_hash = hash<size_t>{}((size_t)key.mDepthMode);
			size_t blend_mode_hash = hash<size_t>{}((size_t)key.mBlendMode);
			size_t cull_winding_hash = hash<size_t>{}((size_t)key.mCullWindingOrder);
			size_t color_format_hash = hash<size_t>{}((size_t)key.mColorFormat);
			size_t depth_format_hash = hash<size_t>{}((size_t)key.mDepthFormat);

			return shader_hash ^ draw_mode_hash ^ depth_mode_hash ^ blend_mode_hash ^ cull_winding_hash ^ color_format_hash ^ depth_format_hash;
		}
	};
}

