/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#define GLM_FORCE_SWIZZLE

// Local Includes
#include "particlevolumecomponent.h"

// External Includes
#include <entity.h>
#include <rect.h>
#include <glm/gtx/transform.hpp>
#include <mathutils.h>
#include <nap/core.h>
#include <renderservice.h>
#include <renderglobals.h>
#include <nap/logger.h>
#include <descriptorsetcache.h>

RTTI_BEGIN_CLASS(nap::ParticleVolumeComponent)
	RTTI_PROPERTY("ComputeMaterialInstance",	&nap::ParticleVolumeComponent::mComputeMaterial,			nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("NumParticles",				&nap::ParticleVolumeComponent::mNumParticles,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Position",					&nap::ParticleVolumeComponent::mPosition,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Size",						&nap::ParticleVolumeComponent::mSize,						nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Rotation",					&nap::ParticleVolumeComponent::mRotation,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RotationVariation",			&nap::ParticleVolumeComponent::mRotationVariation,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RotationSpeed",				&nap::ParticleVolumeComponent::mRotationSpeed,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Spread",						&nap::ParticleVolumeComponent::mSpread,						nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("Velocity",					&nap::ParticleVolumeComponent::mVelocity,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("VelocityVariation",			&nap::ParticleVolumeComponent::mVelocityVariation,			nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ParticleVolumeComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	//////////////////////////////////////////////////////////////////////////
	// Constants
	//////////////////////////////////////////////////////////////////////////

	namespace uniform
	{
		constexpr const char* uboStruct = "UBO";
		constexpr const char* deltaTime = "deltaTime";
		constexpr const char* elapsedTime = "elapsedTime";
		constexpr const char* particleCount = "particleCount";
		constexpr const char* positionBufferStruct = "PositionBuffer";
		constexpr const char* velocityBufferStruct = "VelocityBuffer";
		constexpr const char* rotationBufferStruct = "RotationBuffer";
		constexpr const char* vertexBufferStruct = "VertexBuffer";
	}


	namespace vertexid
	{
		constexpr const char* velocity = "Velocity";
		constexpr const char* id = "Id";
	}


	//////////////////////////////////////////////////////////////////////////
	// Static functions and data
	//////////////////////////////////////////////////////////////////////////

	// Default normalized plane uv's
	static glm::vec4 plane_uvs[] =
	{
		{ 0.0f,	0.0f, 0.0f, 0.0f },
		{ 1.0f,	0.0f, 0.0f, 0.0f },
		{ 0.0f,	1.0f, 0.0f, 0.0f },
		{ 1.0f,	1.0f, 0.0f, 0.0f },
	};

	//////////////////////////////////////////////////////////////////////////

	/**
	 * A particle mesh that is populated by the ParticleEmitterComponent
	 */
	class ParticleMesh : public IMesh
	{
	public:
		int	mNumParticles = 1024;

		ParticleMesh(Core& core) : mRenderService(core.getService<RenderService>())
		{ }

		bool init(utility::ErrorState& errorState)
		{
			assert(mRenderService != nullptr);
			mMeshInstance = std::make_unique<MeshInstance>(*mRenderService);

			int num_vertices = mNumParticles * 4;
			mMeshInstance->setNumVertices(num_vertices);
			mMeshInstance->setUsage(EMeshDataUsage::Static);
			mMeshInstance->setDrawMode(EDrawMode::Triangles);
			mMeshInstance->setCullMode(ECullMode::None);

			// Create the necessary attributes
			Vec4VertexAttribute& position_attribute = mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::position);
			Vec4VertexAttribute& uv_attribute = mMeshInstance->getOrCreateAttribute<glm::vec4>(vertexid::getUVName(0));
			IntVertexAttribute& id_attribute = mMeshInstance->getOrCreateAttribute<int>(vertexid::id);
			MeshShape& shape = mMeshInstance->createShape();

			// Reserve CPU memory for all the particle geometry necessary to create
			// We want to draw the mesh as a set of triangles, 2 triangles per particle
			shape.reserveIndices(mNumParticles);

			// Build the mesh based on the amount of particles
			std::vector<glm::vec4> zero_buffer(num_vertices);
			position_attribute.setData(zero_buffer.data(), num_vertices);

			uint cur_num_vertices = 0;
			for (int i = 0; i < mNumParticles; i++)
			{
				uv_attribute.addData(plane_uvs, 4);

				int id = (cur_num_vertices - cur_num_vertices % 4)/4;
				std::array<int, 4> id_array = { id, id, id, id };
				id_attribute.addData(id_array.data(), 4);

				// Indices for 2 triangles, 1 plane
				unsigned int indices[] =
				{
					cur_num_vertices + 0,
					cur_num_vertices + 1,
					cur_num_vertices + 3,
					cur_num_vertices + 0,
					cur_num_vertices + 3,
					cur_num_vertices + 2
				};
				shape.addIndices(indices, 6);
				cur_num_vertices += 4;
			}

			// Initialize our instance
			return mMeshInstance->init(errorState);
		}

		/**
		* @return MeshInstance as created during init().
		*/
		virtual MeshInstance& getMeshInstance()	override				{ return *mMeshInstance; }

		/**
		* @return MeshInstance as created during init().
		*/
		virtual const MeshInstance& getMeshInstance() const	override	{ return *mMeshInstance; }

	private:
		std::unique_ptr<MeshInstance> mMeshInstance = nullptr;			///< The mesh instance to construct
		nap::RenderService* mRenderService = nullptr;					///< Handle to the render service
	};

	//////////////////////////////////////////////////////////////////////////

	ParticleVolumeComponentInstance::ParticleVolumeComponentInstance(EntityInstance& entity, Component& resource) :
		RenderableMeshComponentInstance(entity, resource),
		mParticleMesh(std::make_unique<ParticleMesh>(*entity.getCore())),
		mRenderService(entity.getCore()->getService<RenderService>())
	{ }


	bool ParticleVolumeComponentInstance::init(utility::ErrorState& errorState)
	{
		// initialize base class
		if (!RenderableMeshComponentInstance::init(errorState))
			return false;

		// Get resource
		ParticleVolumeComponent* resource = getComponent<ParticleVolumeComponent>();
		mParticleSize = resource->mSize;
		mRotationSpeed = resource->mRotationSpeed;

		// Initialize particle mesh
		mParticleMesh->mNumParticles = resource->mNumParticles;
		if (!errorState.check(mParticleMesh->init(errorState), "Unable to create particle mesh"))
			return false;

		// Bind the particle mesh to the material and create a VAO
		RenderableMesh renderableMesh = createRenderableMesh(*mParticleMesh, errorState);
		if (!renderableMesh.isValid())
			return false;

		// Set the particle mesh to be used when drawing
		setMesh(renderableMesh);

		// Create compute instance
		mComputeInstance = std::make_unique<ComputeInstance>(resource->mComputeMaterial, mRenderService);
		if (!errorState.check(mComputeInstance->init(errorState), "Failed to initialize compute instance"))
			return false;

		// Set uniforms
		UniformStructInstance* ubo_struct = mComputeInstance->getComputeMaterialInstance().getOrCreateUniform(uniform::uboStruct);
		if (ubo_struct != nullptr)
		{
			// Cache
			mParticleCountUniform = ubo_struct->getOrCreateUniform<UniformIntInstance>(uniform::particleCount);
			mElapsedTimeUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>(uniform::elapsedTime);
			mDeltaTimeUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>(uniform::deltaTime);
			mVelocityTimeScaleUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>("velocityTimeScale");
			mVelocityVariationScaleUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>("velocityVariationScale");
			mRotationSpeedUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>("rotationSpeed");
			mParticleSizeUniform = ubo_struct->getOrCreateUniform<UniformFloatInstance>("particleSize");

			// Set
			mParticleCountUniform->setValue(mParticleMesh->mNumParticles);
		}

		// Acquire vertex buffer uniform
		UniformStructInstance* vertex_struct = mComputeInstance->getComputeMaterialInstance().getOrCreateUniform(uniform::vertexBufferStruct);
		mVertexBufferUniform = vertex_struct->getOrCreateUniform<UniformVec4BufferInstance>("vertices");

		return true;
	}


	void ParticleVolumeComponentInstance::update(double deltaTime)
	{
		mElapsedTime += deltaTime;

		// Set uniforms
		mDeltaTimeUniform->setValue(static_cast<float>(deltaTime));
		mElapsedTimeUniform->setValue(static_cast<float>(mElapsedTime));

		mVelocityTimeScaleUniform->setValue(mVelocityTimeScale);
		mVelocityVariationScaleUniform->setValue(mVelocityVariationScale);
		mRotationSpeedUniform->setValue(mRotationSpeed);
		mParticleSizeUniform->setValue(mParticleSize);
	}


	bool ParticleVolumeComponentInstance::compute(utility::ErrorState& errorState)
	{
		if (!mComputeInstance->compute(mParticleMesh->mNumParticles, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, errorState))
			return false;

		return true;
	}


	void ParticleVolumeComponentInstance::onDraw(IRenderTarget& renderTarget, VkCommandBuffer commandBuffer, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
	{
		// Get material to work with
		if (!mRenderableMesh.isValid())
		{
			assert(false);
			return;
		}

		// Set mvp matrices if present in material
		if (mProjectMatUniform != nullptr)
			mProjectMatUniform->setValue(projectionMatrix);

		if (mViewMatUniform != nullptr)
			mViewMatUniform->setValue(viewMatrix);

		if (mModelMatUniform != nullptr)
			mModelMatUniform->setValue(mTransformComponent->getGlobalTransform());

		// Acquire new / unique descriptor set before rendering
		MaterialInstance& mat_instance = getMaterialInstance();
		VkDescriptorSet descriptor_set = mat_instance.update();

		// Fetch and bind pipeline
		utility::ErrorState error_state;
		RenderService::Pipeline pipeline = mRenderService->getOrCreatePipeline(renderTarget, mRenderableMesh.getMesh(), mat_instance, error_state);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mPipeline);

		// Bind shader descriptors
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.mLayout, 0, 1, &descriptor_set, 0, nullptr);

		// Get storage buffer from uniform
		const VkBuffer storage_buffer = mVertexBufferUniform->getTypedValueBuffer().getBuffer();

		// Bind vertex buffers
		const std::vector<VkBuffer>& vertex_buffers = mRenderableMesh.getVertexBuffers();
		const std::vector<VkDeviceSize>& offsets = mRenderableMesh.getVertexBufferOffsets();

		std::vector<VkBuffer> vertex_buffers_override = { storage_buffer, vertex_buffers[1], vertex_buffers[2] };
		//vkCmdBindVertexBuffers(commandBuffer, 0, vertex_buffers.size(), vertex_buffers.data(), offsets.data());
		vkCmdBindVertexBuffers(commandBuffer, 0, vertex_buffers_override.size(), vertex_buffers_override.data(), offsets.data());

		// TODO: move to push/pop cliprect on RenderTarget once it has been ported
		bool has_clip_rect = mClipRect.hasWidth() && mClipRect.hasHeight();
		if (has_clip_rect)
		{
			VkRect2D rect;
			rect.offset.x = mClipRect.getMin().x;
			rect.offset.y = mClipRect.getMin().y;
			rect.extent.width = mClipRect.getWidth();
			rect.extent.height = mClipRect.getHeight();
			vkCmdSetScissor(commandBuffer, 0, 1, &rect);
		}

		// Set line width
		vkCmdSetLineWidth(commandBuffer, mLineWidth);

		// Draw meshes
		MeshInstance& mesh_instance = getMeshInstance();
		GPUMesh& mesh = mesh_instance.getGPUMesh();
		for (int index = 0; index < mesh_instance.getNumShapes(); ++index)
		{
			const IndexBuffer& index_buffer = mesh.getIndexBuffer(index);
			vkCmdBindIndexBuffer(commandBuffer, index_buffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, index_buffer.getCount(), 1, 0, 0, 0);
		}

		// Restore line width
		vkCmdSetLineWidth(commandBuffer, 1.0f);

		// Restore clipping
		if (has_clip_rect)
		{
			VkRect2D rect;
			rect.offset.x = 0;
			rect.offset.y = 0;
			rect.extent.width = renderTarget.getBufferSize().x;
			rect.extent.height = renderTarget.getBufferSize().y;
			vkCmdSetScissor(commandBuffer, 0, 1, &rect);
		}
	}
}
