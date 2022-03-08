/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local Includes
#include "irendertarget.h"
#include "depthtexture2d.h"

// External Includes
#include <nap/resource.h>
#include <nap/resourceptr.h>
#include <vulkan/vulkan_core.h>

namespace nap
{
	// Forward Declares
	class RenderTexture2D;
	class RenderService;

	/**
	 * Renders to a depth attachment exclusively. Skips the fragment shader of all material instances.
	 * 
	 * A resource that is used to render one or multiple objects to a nap::RenderTexture2D instead of a nap::RenderWindow.
	 * This objects requires a link to a nap::RenderTexture2D to store the result of the render pass.
	 * Only render to a render target within a headless recording pass, failure to do so will result in undefined behavior.
	 * Make sure to call beginRendering() to start the render pass and endRendering() to end the render pass.
	 * Always call RenderService::endHeadlessRecording after having recorded all off-screen render operations.
	 *
	 * ~~~~~{.cpp} 
	 *		mRenderService->beginFrame();
	 *		if (mRenderService->beginHeadlessRecording())
	 *		{
	 *			...
	 *			mTargetOne->beginRendering();
	 *			mRenderService->renderObjects(*mTargetOne, ortho_cam, objects_one);
	 *			mTargetOne->endRendering();
	 *			...
	 *			mTargetTwo->beginRendering();
	 *			mRenderService->renderObjects(*mTargetTwo, ortho_cam, objects_two);
	 *			mTargetTwo->endRendering();
	 *			...
	 *			mRenderService->endHeadlessRecording();
	 *		}
	 *		mRenderService->endFrame();
	 * ~~~~~
	 *
	 */
	class NAPAPI DepthRenderTarget : public Resource, public IRenderTarget
	{
		RTTI_ENABLE(Resource)
	public:
		/**
		 * Every render target requires a reference to core.
		 * @param core link to a nap core instance
		 */
		DepthRenderTarget(Core& core);
		
		/**
		 * Destroys allocated render resources
		 */
		~DepthRenderTarget();

		/**
		 * Initializes the render target, including all the required resources.
		 * @param errorState contains the error if initialization failed.
		 * @return if initialization succeeded.
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * Starts the render pass.
		 * Only start the render pass after a successful call to RenderService::beginHeadlessRecording().
		 *
		 * ~~~~~{.cpp}
		 *		mRenderService->beginFrame();
		 *		if (mRenderService->beginHeadlessRecording())
		 *		{
		 *			...
		 *			mTarget->beginRendering();
		 *			mRenderService->renderObjects(*mTarget, ortho_cam, objects_one);
		 *			mTarget->endRendering();
		 *			...
		 *			mRenderService->endHeadlessRecording();
		 *		}
		 *		mRenderService->endFrame();
		 * ~~~~~
		 */
		virtual void beginRendering() override;

		/**
		 * Ends the render pass. Always call this after beginRendering().
		 *
		 * ~~~~~{.cpp}
		 *		mRenderService->beginFrame();
		 *		if (mRenderService->beginHeadlessRecording())
		 *		{
		 *			...
		 *			mTarget->beginRendering();
		 *			mTarget->renderObjects(*mTarget, ortho_cam, objects_one);
		 *			mTarget->endRendering();
		 *			...
		 *			mRenderService->endHeadlessRecording();
		 *		}
		 *		mRenderService->endFrame();
		 * ~~~~~
		*/
		virtual void endRendering() override;

		/**
		 * @return size in pixels of the render target.
		 */
		virtual const glm::ivec2 getBufferSize() const override;

		/**
		 * Updates the render target clear color.
		 * @param color the new clear color to use.
		 */
		virtual void setClearColor(const RGBAColorFloat& color) override		{ mClearValue = color.getRed(); }
		
		/**
		 * @return the currently used render target clear color.
		 */
		virtual const RGBAColorFloat& getClearColor() const override			{ return mClearColor; }

		/**
		 * Geometry winding order, defaults to clockwise. 
		 */
		virtual ECullWindingOrder getWindingOrder() const override				{ return ECullWindingOrder::Clockwise; }

		/**
		 * @return the render pass
		 */
		virtual VkRenderPass getRenderPass() const override						{ return mRenderPass; }

		/**
		 * @return used number of samples when rendering to the target.
		 */
		virtual VkSampleCountFlagBits getSampleCount() const override			{ return VK_SAMPLE_COUNT_1_BIT; }

		/**
		 * @return if sample based shading is enabled when rendering to the target.
		 */
		virtual bool getSampleShadingEnabled() const override					{ return false; };

		/**
		 * @return the texture that holds the result of the render pass.
		 */
		DepthTexture2D& getDepthTexture();

		/**
		 * @return render target color format. This is the format of the linked in color texture.
		 */
		virtual VkFormat getColorFormat() const override;

		/**
		 * @return render target depth format
		 */
		virtual VkFormat getDepthFormat() const override;

	public:
		float							mClearValue = 1.0f;									///< Property: 'ClearValue' value selection used for clearing the render target
		ResourcePtr<DepthTexture2D>		mDepthTexture;										///< Property: 'DepthTexture' texture to render to, format needs to be: 'Backbuffer'

	private:
		RenderService*					mRenderService;
		VkFramebuffer					mFramebuffer = VK_NULL_HANDLE;
		VkRenderPass					mRenderPass = VK_NULL_HANDLE;
		VkSampleCountFlagBits			mRasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		RGBAColorFloat					mClearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
}
