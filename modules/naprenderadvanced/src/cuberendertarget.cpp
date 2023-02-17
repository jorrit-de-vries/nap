/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "cuberendertarget.h"

// Nap includes
#include <renderservice.h>
#include <textureutils.h>
#include <nap/core.h>
#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::CubeRenderTarget)
	RTTI_CONSTRUCTOR(nap::Core&)
	//RTTI_PROPERTY("ColorTexture",			&nap::CubeRenderTarget::mColorTexture,				nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Width",					&nap::CubeRenderTarget::mWidth,						nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Height",					&nap::CubeRenderTarget::mHeight,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("SampleShading",			&nap::CubeRenderTarget::mSampleShading,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Samples",				&nap::CubeRenderTarget::mRequestedSamples,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("ClearColor",				&nap::CubeRenderTarget::mClearColor,				nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

namespace nap
{
    //////////////////////////////////////////////////////////////////////////
    // Static functions
    //////////////////////////////////////////////////////////////////////////

	bool createCubeRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples, VkImageLayout targetLayout, VkRenderPass& renderPass, utility::ErrorState& errorState)
	{
		if (!errorState.check(targetLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, "Failed to create render pass. Unsupported target layout."))
			return false;

		bool multi_sample = samples != VK_SAMPLE_COUNT_1_BIT;

		VkAttachmentDescription color_attachment = {};
		color_attachment.format = colorFormat;
		color_attachment.samples = samples;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = !multi_sample ? targetLayout : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.finalLayout = !multi_sample ? targetLayout : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = depthFormat;
		depth_attachment.samples = samples;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderpass_info = {};
		renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;
		renderpass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderpass_info.pDependencies = dependencies.data();

		// Single-sample render pass
		if (!multi_sample)
		{
			std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
			renderpass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderpass_info.pAttachments = attachments.data();

			return errorState.check(vkCreateRenderPass(device, &renderpass_info, nullptr, &renderPass) == VK_SUCCESS, "Failed to create render pass");
		}

		// Multi-sample render pass
		else
		{
			VkAttachmentDescription resolve_attachment{};
			resolve_attachment.format = colorFormat;
			resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolve_attachment.initialLayout = targetLayout;
			resolve_attachment.finalLayout = targetLayout;

			VkAttachmentReference resolve_attachment_ref{};
			resolve_attachment_ref.attachment = 2;
			resolve_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			subpass.pResolveAttachments = &resolve_attachment_ref;

			std::array<VkAttachmentDescription, 3> attachments = { color_attachment, depth_attachment, resolve_attachment };
			renderpass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderpass_info.pAttachments = attachments.data();

			return errorState.check(vkCreateRenderPass(device, &renderpass_info, nullptr, &renderPass) == VK_SUCCESS, "Failed to create multi-sample render pass");
		}
	}


	// Creates the color image and view
	static bool createColorResource(const RenderService& renderer, VkExtent2D targetSize, VkFormat colorFormat, VkSampleCountFlagBits sampleCount, ImageData& outData, utility::ErrorState& errorState)
	{
		if (!create2DImage(renderer.getVulkanAllocator(), targetSize.width, targetSize.height, colorFormat, 1, sampleCount, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, outData.mImage, outData.mAllocation, outData.mAllocationInfo, errorState))
			return false;

		if (!create2DImageView(renderer.getDevice(), outData.getImage(), colorFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT, outData.mView, errorState))
			return false;

		return true;
	}


	// Create the depth image and view
	static bool createDepthResource(const RenderService& renderer, VkExtent2D targetSize, VkFormat depthFormat, VkSampleCountFlagBits sampleCount, ImageData& outImage, utility::ErrorState& errorState)
	{
		if (!create2DImage(renderer.getVulkanAllocator(), targetSize.width, targetSize.height, depthFormat, 1, sampleCount, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, outImage.mImage, outImage.mAllocation, outImage.mAllocationInfo, errorState))
			return false;

		if (!create2DImageView(renderer.getDevice(), outImage.getImage(), renderer.getDepthFormat(), 1, VK_IMAGE_ASPECT_DEPTH_BIT, outImage.mView, errorState))
			return false;

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// CubeRenderTarget
	//////////////////////////////////////////////////////////////////////////

	CubeRenderTarget::CubeRenderTarget(Core& core) :
		mRenderService(core.getService<RenderService>())
	{}


	CubeRenderTarget::~CubeRenderTarget()
	{
		for (auto& fb : mFramebuffers)
		{
			if (fb != VK_NULL_HANDLE)
				vkDestroyFramebuffer(mRenderService->getDevice(), fb, nullptr);
		}

		if (mRenderPass != nullptr)
			vkDestroyRenderPass(mRenderService->getDevice(), mRenderPass, nullptr);

		for (auto& img : mDepthImages)
			destroyImageAndView(img, mRenderService->getDevice(), mRenderService->getVulkanAllocator());

		for (auto& img : mColorImages)
			destroyImageAndView(img, mRenderService->getDevice(), mRenderService->getVulkanAllocator());
	}


	bool CubeRenderTarget::init(utility::ErrorState& errorState)
	{
		// Warn if requested number of samples is not matched by hardware
		if (!mRenderService->getRasterizationSamples(mRequestedSamples, mRasterizationSamples, errorState))
			nap::Logger::warn(errorState.toString().c_str());

		// Check if sample rate shading is enabled
		if (mSampleShading && !(mRenderService->sampleShadingSupported()))
		{
			nap::Logger::warn("Sample shading requested but not supported");
			mSampleShading = false;
		}

		// Set size
		mSize = { mWidth, mHeight };

		SurfaceDescriptor color_settings;
		color_settings.mWidth = mWidth;
		color_settings.mHeight = mHeight;
		color_settings.mColorSpace = EColorSpace::Linear;
		color_settings.mChannels = ESurfaceChannels::RGBA;
		color_settings.mDataType = ESurfaceDataType::BYTE;
		mVulkanColorFormat = getTextureFormat(color_settings);

		SurfaceDescriptor depth_settings = color_settings;
		color_settings.mChannels = ESurfaceChannels::D;
		mVulkanDepthFormat = getTextureFormat(depth_settings);

		// Framebuffer and attachment sizes
		VkExtent2D framebuffer_size = { mWidth, mHeight };

		// Create framebuffer info
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.width = framebuffer_size.width;
		framebuffer_info.height = framebuffer_size.height;
		framebuffer_info.layers = 1;

		// Create render pass based on number of multi samples
		// When there's only 1 there's no need for a resolve step
		if (!createCubeRenderPass(mRenderService->getDevice(), mVulkanColorFormat, getTextureFormat(depth_settings),
			mRasterizationSamples, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mRenderPass, errorState))
			return false;

		for (uint i = 0; i < LAYER_COUNT; i++)
		{
			if (mRasterizationSamples == VK_SAMPLE_COUNT_1_BIT)
			{
				// Create depth attachments
				for (auto& img : mDepthImages)
				{
					if (!createDepthResource(*mRenderService, framebuffer_size, mVulkanDepthFormat, mRasterizationSamples, img, errorState))
						return false;
				}

				std::array<VkImageView, 2> attachments{ mColorImages[i].getView(), mDepthImages[i].getView() };
				framebuffer_info.pAttachments = attachments.data();
				framebuffer_info.attachmentCount = attachments.size();
				framebuffer_info.renderPass = mRenderPass;

				// Create framebuffer
				if (!errorState.check(vkCreateFramebuffer(mRenderService->getDevice(), &framebuffer_info, nullptr, &mFramebuffers[i]) == VK_SUCCESS, "Failed to create framebuffer"))
					return false;
			}
			else
			{
				NAP_ASSERT_MSG(false, "Multisampled cube render targets not yet implemented");
				//// Create multi-sampled color attachments
				//for (auto& img : mColorImages)
				//{
				//	if (!createColorResource(*mRenderService, framebuffer_size, getTextureFormat(color_settings), mRasterizationSamples, img, errorState))
				//		return false;
				//}

				//// Create multi-sampled depth attachment
				//if (!createDepthResource(*mRenderService, framebuffer_size, mRasterizationSamples, mDepthImage, errorState))
				//	return false;

				//std::array<VkImageView, 3> attachments{ mColorImage.mTextureView, mDepthImage.mTextureView, mColorTexture->getImageView() };
				//framebuffer_info.pAttachments = attachments.data();
				//framebuffer_info.attachmentCount = attachments.size();
				//framebuffer_info.renderPass = mRenderPass;

				//// Create a framebuffer that links the cell target texture to the appropriate resolved color attachment
				//if (!errorState.check(vkCreateFramebuffer(mRenderService->getDevice(), &framebuffer_info, nullptr, &mFramebuffers[i]) == VK_SUCCESS, "Failed to create framebuffer"))
				//	return false;
			}
		}
		return true;
	}


	void CubeRenderTarget::beginRendering()
	{
		// We transition the layout of the depth attachment from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL, once in the first pass
		if (mIsFirstPass)
		{
			for (auto& img : mDepthImages)
			{
				transitionDepthImageLayout(mRenderService->getCurrentCommandBuffer(), img.mImage,
					img.mCurrentLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1);
			}
		}

		const RGBAColorFloat& clear_color = mClearColor;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { clear_color[0], clear_color[1], clear_color[2], clear_color[3] };
		clearValues[1].depthStencil = { 1.0f, 0 };

		const glm::ivec2 offset = { 0, 0 };

		// Setup render pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = VK_NULL_HANDLE; // mFramebuffer;
		renderPassInfo.renderArea.offset = { offset.x, offset.y };
		renderPassInfo.renderArea.extent = { static_cast<uint32_t>(mSize.x), static_cast<uint32_t>(mSize.y) };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Begin render pass
		vkCmdBeginRenderPass(mRenderService->getCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Ensure scissor and viewport are covering the cell area
		VkRect2D rect = {};
		rect.offset = { offset.x, offset.y };
		rect.extent = { (uint32_t)mSize.x, (uint32_t)mSize.y };
		vkCmdSetScissor(mRenderService->getCurrentCommandBuffer(), 0, 1, &rect);

		VkViewport viewport = {};
		viewport.x = static_cast<float>(offset.x);
		viewport.y = mSize.y + static_cast<float>(offset.y);
		viewport.width = mSize.x;
		viewport.height = -mSize.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(mRenderService->getCurrentCommandBuffer(), 0, 1, &viewport);
	}


	void CubeRenderTarget::endRendering()
	{
		vkCmdEndRenderPass(mRenderService->getCurrentCommandBuffer());
	}


	void CubeRenderTarget::render(QuiltCameraComponentInstance& quiltCamera, std::function<void(nap::CubeRenderTarget&, nap::QuiltCameraComponentInstance&)> renderCallback)
	{
		// Render to frame buffers
		//const QuiltSettings& settings = mDevice->getQuiltSettings();

		//for (int i = 0; i < settings.getViewCount(); i++)
		//{
		//	mCurrentView = i;
		//	quiltCamera.setView(i, settings.getViewCount());

		//	beginRendering();
		//	renderCallback(*this, quiltCamera);
		//	endRendering();
		//}
		//quiltCamera.resetView();
		//mIsFirstPass = false;
	}
}
