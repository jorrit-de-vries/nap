/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "materialinstance.h"
#include "mesh.h"
#include "material.h"
#include "renderservice.h"
#include "vk_mem_alloc.h"
#include "renderutils.h"

// External includes
#include <nap/logger.h>
#include <rtti/rttiutilities.h>

RTTI_DEFINE_BASE(nap::BaseMaterialInstanceResource)

RTTI_BEGIN_CLASS(nap::MaterialInstanceResource)
	RTTI_PROPERTY("Material",					&nap::MaterialInstanceResource::mMaterial,					nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Uniforms",					&nap::MaterialInstanceResource::mUniforms,					nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Bindings",					&nap::MaterialInstanceResource::mBufferBindings,			nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Samplers",					&nap::MaterialInstanceResource::mSamplers,					nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("BlendMode",					&nap::MaterialInstanceResource::mBlendMode,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("DepthMode",					&nap::MaterialInstanceResource::mDepthMode,					nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::ComputeMaterialInstanceResource)
	RTTI_PROPERTY("ComputeMaterial",			&nap::ComputeMaterialInstanceResource::mComputeMaterial,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Uniforms",					&nap::ComputeMaterialInstanceResource::mUniforms,			nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Bindings",					&nap::ComputeMaterialInstanceResource::mBufferBindings,		nap::rtti::EPropertyMetaData::Embedded)
	RTTI_PROPERTY("Samplers",					&nap::ComputeMaterialInstanceResource::mSamplers,			nap::rtti::EPropertyMetaData::Embedded)
RTTI_END_CLASS

RTTI_DEFINE_BASE(nap::BaseMaterialInstance)

RTTI_BEGIN_CLASS(nap::MaterialInstance)
	RTTI_FUNCTION("getOrCreateUniform",	(nap::UniformStructInstance* (nap::MaterialInstance::*)(const std::string&)) &nap::MaterialInstance::getOrCreateUniform);
	RTTI_FUNCTION("getOrCreateSampler",	(nap::SamplerInstance* (nap::MaterialInstance::*)(const std::string&)) &nap::MaterialInstance::getOrCreateSampler);
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::ComputeMaterialInstance)
	RTTI_FUNCTION("getOrCreateUniform", (nap::UniformStructInstance* (nap::ComputeMaterialInstance::*)(const std::string&))& nap::ComputeMaterialInstance::getOrCreateUniform);
	RTTI_FUNCTION("getOrCreateSampler", (nap::SamplerInstance* (nap::ComputeMaterialInstance::*)(const std::string&))& nap::ComputeMaterialInstance::getOrCreateSampler);
RTTI_END_CLASS

namespace nap
{
	template<class T>
	const UniformInstance* findUniformStructInstanceMember(const std::vector<T>& members, const std::string& name)
	{
		for (auto& member : members)
			if (member->getDeclaration().mName == name)
				return member.get();

		return nullptr;
	}


	template<class T>
	const BufferBinding* findBindingResource(const std::vector<T>& bindings, const ShaderVariableDeclaration& declaration)
	{
		for (auto& binding : bindings)
			if (binding->mName == declaration.mName)
				return binding.get();

		return nullptr;
	}


	template<class T>
	const Sampler* findSamplerResource(const std::vector<T>& samplers, const SamplerDeclaration& declaration)
	{
		for (auto& sampler : samplers)
			if (sampler->mName == declaration.mName)
				return sampler.get();

		return nullptr;
	}


	void buildUniformBufferObjectRecursive(const UniformStructInstance& baseInstance, const UniformStructInstance* overrideInstance, UniformBufferObject& ubo)
	{
		for (auto& base_uniform : baseInstance.getUniforms())
		{
			rtti::TypeInfo declaration_type = base_uniform->get_type();

			const UniformInstance* override_uniform = nullptr;
			if (overrideInstance != nullptr)
				override_uniform = findUniformStructInstanceMember(overrideInstance->getUniforms(), base_uniform->getDeclaration().mName);

			if (declaration_type == RTTI_OF(UniformStructArrayInstance))
			{
				const UniformStructArrayInstance* struct_array_override = rtti_cast<const UniformStructArrayInstance>(override_uniform);
				const UniformStructArrayInstance* struct_array_declaration = rtti_cast<const UniformStructArrayInstance>(base_uniform.get());

				int resource_index = 0;
				for (auto& base_element : struct_array_declaration->getElements())
				{
					UniformStructInstance* element_override = nullptr;
					if (struct_array_override != nullptr && resource_index < struct_array_override->getElements().size())
						element_override = struct_array_override->getElements()[resource_index++].get();

					buildUniformBufferObjectRecursive(*base_element, element_override, ubo);
				}
			}
			else if (declaration_type.is_derived_from(RTTI_OF(UniformValueArrayInstance)))
			{
				const UniformValueArrayInstance* base_array_uniform = rtti_cast<const UniformValueArrayInstance>(base_uniform.get());
				const UniformValueArrayInstance* override_array_uniform = rtti_cast<const UniformValueArrayInstance>(override_uniform);

				if (override_array_uniform != nullptr)
					ubo.mUniforms.push_back(override_array_uniform);
				else
					ubo.mUniforms.push_back(base_array_uniform);
			}
			else if (declaration_type == RTTI_OF(UniformStructInstance))
			{
				const UniformStructInstance* base_struct = rtti_cast<const UniformStructInstance>(base_uniform.get());
				const UniformStructInstance* override_struct = rtti_cast<const UniformStructInstance>(override_uniform);

				buildUniformBufferObjectRecursive(*base_struct, override_struct, ubo);
			}
			else
			{
				const UniformValueInstance* base_value = rtti_cast<const UniformValueInstance>(base_uniform.get());
				const UniformValueInstance* override_value = rtti_cast<const UniformValueInstance>(override_uniform);

				if (override_value != nullptr)
					ubo.mUniforms.push_back(override_value);
				else
					ubo.mUniforms.push_back(base_value);
			}
		}
	}


	void updateUniforms(const DescriptorSet& descriptorSet, std::vector<UniformBufferObject>& bufferObjects)
	{
		// Go over all the UBOs and memcpy the latest MaterialInstance state into the allocated descriptorSet's VkBuffers
		for (int ubo_index = 0; ubo_index != descriptorSet.mBuffers.size(); ++ubo_index)
		{
			UniformBufferObject& ubo = bufferObjects[ubo_index];
			VmaAllocationInfo allocation = descriptorSet.mBuffers[ubo_index].mAllocationInfo;

			void* mapped_memory = allocation.pMappedData;
			for (auto& uniform : ubo.mUniforms)
			{
				uniform->push((uint8_t*)mapped_memory);
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// BaseMaterialInstance
	//////////////////////////////////////////////////////////////////////////

	UniformStructInstance* BaseMaterialInstance::getOrCreateUniform(const std::string& name)
	{
		UniformStructInstance* existing = findUniform(name);
		if (existing != nullptr)
			return existing;

		// Find the declaration in the shader (if we can't find it, it's not a name that actually exists in the shader, which is an error).
		const ShaderVariableStructDeclaration* declaration = nullptr;
		const std::vector<BufferObjectDeclaration>& ubo_declarations = getBaseMaterial()->getBaseShader()->getUBODeclarations();
		for (const BufferObjectDeclaration& ubo_declaration : ubo_declarations)
		{
			if (ubo_declaration.mName == name)
			{
				declaration = &ubo_declaration;
				break;
			}
		}

		if (declaration == nullptr)
			return nullptr;

		// At the MaterialInstance level, we always have UBOs at the root, so we create a root struct
		return &createUniformRootStruct(*declaration, std::bind(&BaseMaterialInstance::onUniformCreated, this));
	}


	BufferBindingInstance* BaseMaterialInstance::getOrCreateBufferBinding(const std::string& name)
	{
		// See if we have an override in MaterialInstance. If so, we can return it
		BufferBindingInstance* existing_binding = findBufferBinding(name);
		if (existing_binding != nullptr)
			return existing_binding;

		// Find the declaration in the shader (if we can't find it, it's not a name that actually exists in the shader, which is an error).
		const ShaderVariableStructDeclaration* declaration = nullptr;
		const std::vector<BufferObjectDeclaration>& ssbo_declarations = getBaseMaterial()->getBaseShader()->getSSBODeclarations();
		utility::ErrorState error_state;

		int ssbo_index = 0;
		for (const BufferObjectDeclaration& declaration : ssbo_declarations)
		{
			if (declaration.mName == name)
			{
				// Get the first and only member of the declaration
				const auto& binding_declaration = declaration.getBufferDeclaration();

				std::unique_ptr<BufferBindingInstance> binding_instance_override;
				binding_instance_override = BufferBindingInstance::createBufferBindingInstanceFromDeclaration(binding_declaration, nullptr, std::bind(&BaseMaterialInstance::onBindingChanged, this, ssbo_index, std::placeholders::_1), error_state);
				NAP_ASSERT_MSG(binding_instance_override != nullptr, error_state.toString().c_str());

				return &addBufferBindingInstance(std::move(binding_instance_override));	
			}
			++ssbo_index;
		}
		return nullptr;
	}


	SamplerInstance* BaseMaterialInstance::getOrCreateSamplerInternal(const std::string& name)
	{
		// See if we have an override in MaterialInstance. If so, we can return it
		SamplerInstance* existing_sampler = findSampler(name);
		if (existing_sampler != nullptr)
			return existing_sampler;

		const BaseShader* shader = getBaseMaterial()->getBaseShader();
		const SamplerDeclarations& sampler_declarations = shader->getSamplerDeclarations();
		int image_start_index = 0;
		for (const SamplerDeclaration& declaration : sampler_declarations)
		{
			if (declaration.mName == name)
			{
				bool is_array = declaration.mNumArrayElements > 1;

				std::unique_ptr<SamplerInstance> sampler_instance_override;
				if (is_array)
					sampler_instance_override = std::make_unique<Sampler2DArrayInstance>(*mRenderService, declaration, nullptr, std::bind(&MaterialInstance::onSamplerChanged, this, image_start_index, std::placeholders::_1));
				else
					sampler_instance_override = std::make_unique<Sampler2DInstance>(*mRenderService, declaration, nullptr, std::bind(&MaterialInstance::onSamplerChanged, this, image_start_index, std::placeholders::_1));

				utility::ErrorState error_state;
				bool initialized = sampler_instance_override->init(error_state);
				assert(initialized);

				return &addSamplerInstance(std::move(sampler_instance_override));
			}
			image_start_index += declaration.mNumArrayElements;
		}
		return nullptr;
	}


	void BaseMaterialInstance::onUniformCreated()
	{
		// We only store that uniforms have been created. During update() we will update UBO structures. The reason
		// why we don't do this in place is because we to avoid multiple rebuilds for a single draw.
		mUniformsCreated = true;
	}


	// This function is called whenever a SamplerInstance changes its texture. We already have VkWriteDescriptorSets
	// that contain image information that point into the mSampleImages array. The information in VkWriteDescriptorSets
	// remains static after init, no matter what textures we use, because it is pointing to indices into the mSamplerImages array.
	// What we need to do here is update the contents of the mSamplerImages array so that it points to correct information
	// for the texture change. This way, when update() is called, VkUpdateDescriptorSets will use the correct image info.
	void BaseMaterialInstance::onSamplerChanged(int imageStartIndex, SamplerInstance& samplerInstance)
	{
		VkSampler vk_sampler = samplerInstance.getVulkanSampler();
		if (samplerInstance.get_type() == RTTI_OF(Sampler2DArrayInstance))
		{
			Sampler2DArrayInstance* sampler_2d_array = (Sampler2DArrayInstance*)(&samplerInstance);

			for (int index = 0; index < sampler_2d_array->getNumElements(); ++index)
			{
				const Texture2D& texture = sampler_2d_array->getTexture(index);

				VkDescriptorImageInfo& imageInfo = mSamplerDescriptors[imageStartIndex + index];
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = texture.getImageView();
				imageInfo.sampler = vk_sampler;
			}
		}
		else
		{
			Sampler2DInstance* sampler_2d = (Sampler2DInstance*)(&samplerInstance);

			VkDescriptorImageInfo& imageInfo = mSamplerDescriptors[imageStartIndex];
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = sampler_2d->getTexture().getImageView();
			imageInfo.sampler = vk_sampler;
		}
	}


	void BaseMaterialInstance::onBindingChanged(int storageBufferIndex, BufferBindingInstance& bindingInstance)
	{
		// Update the buffer info structure stored in the buffer info handles
		VkDescriptorBufferInfo& buffer_info = mStorageDescriptors[storageBufferIndex];
		if (bindingInstance.get_type().is_derived_from(RTTI_OF(BufferBindingStructInstance)))
		{
			BufferBindingStructInstance* instance = static_cast<BufferBindingStructInstance*>(&bindingInstance);
			buffer_info.buffer = instance->getBuffer().getBuffer();
		}
		else if (bindingInstance.get_type().is_derived_from(RTTI_OF(BufferBindingNumericInstance)))
		{
			if (bindingInstance.get_type() == RTTI_OF(BufferBindingUIntInstance))
			{
				auto* instance = static_cast<BufferBindingUIntInstance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingIntInstance))
			{
				auto* instance = static_cast<BufferBindingIntInstance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingFloatInstance))
			{
				auto* instance = static_cast<BufferBindingFloatInstance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingVec2Instance))
			{
				auto* instance = static_cast<BufferBindingVec2Instance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingVec3Instance))
			{
				auto* instance = static_cast<BufferBindingVec3Instance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingVec4Instance))
			{
				auto* instance = static_cast<BufferBindingVec4Instance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
			else if (bindingInstance.get_type() == RTTI_OF(BufferBindingMat4Instance))
			{
				auto* instance = static_cast<BufferBindingMat4Instance*>(&bindingInstance);
				buffer_info.buffer = instance->getBuffer().getBuffer();
			}
		}
		else
		{
			NAP_ASSERT_MSG(false, "Unsupported storage uniform type");
		}
	}


	void BaseMaterialInstance::rebuildUBO(UniformBufferObject& ubo, UniformStructInstance* overrideStruct)
	{
		ubo.mUniforms.clear();

		const UniformStructInstance* base_struct = rtti_cast<const UniformStructInstance>(getBaseMaterial()->findUniform(ubo.mDeclaration->mName));
		assert(base_struct != nullptr);

		buildUniformBufferObjectRecursive(*base_struct, overrideStruct, ubo);
	}


	void BaseMaterialInstance::addImageInfo(const Texture2D& texture2D, VkSampler sampler)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture2D.getImageView();
		imageInfo.sampler = sampler;

		mSamplerDescriptors.push_back(imageInfo);
	}


	bool BaseMaterialInstance::initBindings(utility::ErrorState& errorState)
	{
		// Here we create SSBOs in the same way as we did for UBOs above
		const auto& ssbo_declarations = getBaseMaterial()->getBaseShader()->getSSBODeclarations();
		mStorageDescriptors.resize(ssbo_declarations.size());
		mStorageWriteDescriptorSets.reserve(ssbo_declarations.size()); // We reserve to ensure that pointers remain consistent during the iteration

		int ssbo_index = 0;
		for (const BufferObjectDeclaration& declaration : ssbo_declarations)
		{
			// Verify buffer object type
			if (!errorState.check(declaration.mDescriptorType == EDescriptorType::Storage, utility::stringFormat("Buffer Object Type mismatch in shader declaration %s", declaration.mName.c_str())))
				return false;

			// Check if the binding is set as override in the MaterialInstance
			const BufferBinding* override_resource = findBindingResource(getResource()->mBufferBindings, declaration);

			const auto& buffer_declaration = declaration.getBufferDeclaration();

			BufferBindingInstance* binding = nullptr;
			if (override_resource != nullptr)
			{
				// Buffer binding is overridden, make a BufferBindingInstance object
				auto override_instance = BufferBindingInstance::createBufferBindingInstanceFromDeclaration(declaration, override_resource, std::bind(&BaseMaterialInstance::onBindingChanged, this, ssbo_index, std::placeholders::_1), errorState);
				if (!errorState.check(override_instance != nullptr, "Failed to create buffer binding instance for shader variable `%s`", declaration.mName.c_str()))
					return false;

				if (!errorState.check(override_instance->hasBuffer(), utility::stringFormat("No valid buffer was assigned to shader variable '%s' in material override '%s'", declaration.mName.c_str(), getBaseMaterial()->mID.c_str()).c_str()))
					return false;

				binding = &addBufferBindingInstance(std::move(override_instance));
			}
			else
			{
				// Binding is not overridden, find it in the Material
				binding = findBufferBinding(declaration.mName);
				if (!errorState.check(binding != nullptr, "Failed to find buffer binding instance for shader variable `%s` in base material", declaration.mName.c_str()))
					return false;

				if (!errorState.check(binding->hasBuffer(), utility::stringFormat("No valid buffer was assigned to shader variable '%s' in base material '%s'", declaration.mName.c_str(), getBaseMaterial()->mID.c_str()).c_str()))
					return false;
			}

			VkDescriptorBufferInfo& buffer_info = mStorageDescriptors[ssbo_index];
			buffer_info.buffer = binding->getBaseBuffer().getBuffer();
			buffer_info.offset = 0;
			buffer_info.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet& ssbo_descriptor = mStorageWriteDescriptorSets.emplace_back();
			ssbo_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ssbo_descriptor.dstSet = VK_NULL_HANDLE;
			ssbo_descriptor.dstBinding = declaration.mBinding;
			ssbo_descriptor.dstArrayElement = 0;
			ssbo_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			ssbo_descriptor.descriptorCount = 1;
			ssbo_descriptor.pBufferInfo = mStorageDescriptors.data() + ssbo_index;

			++ssbo_index;
		}
		return true;
	}


	bool BaseMaterialInstance::initSamplers(utility::ErrorState& errorState)
	{
		BaseMaterial* material = getBaseMaterial();
		const SamplerDeclarations& sampler_declarations = material->getBaseShader()->getSamplerDeclarations();

		int num_sampler_images = 0;
		for (const SamplerDeclaration& declaration : sampler_declarations)
			num_sampler_images += declaration.mNumArrayElements;

		mSamplerWriteDescriptorSets.resize(sampler_declarations.size());
		mSamplerDescriptors.reserve(num_sampler_images);	// We reserve to ensure that pointers remain consistent during the iteration

		Texture2D& emptyTexture = mRenderService->getEmptyTexture();

		// Samplers are initialized in two steps (somewhat similar to how uniforms are setup):
		// 1) We create sampler instances based on sampler declarations for all properties in MaterialInstance (so, the ones that are overridden).
		// 2) We initialize a VkWriteDescriptorSet that contains information that is either pointing to data from MaterialInstance if overridden, 
		//    otherwise data from the Material. This VkWriteDescriptorSet is used later to update the final DescriptorSet that will be used for 
		//    rendering. The VkWriteDescriptorSet is not yet complete because the final DescriptorSet is not yet set (it is nullptr). The reason is 
		//    that the DescriptorSet is acquired before rendering (see comments in update()).
		//
		// The VkWriteDescriptorSets require information about the images that are bound in the form of a VkDescriptorImageInfo. Each bound image 
		// needs such a structure. We build these here as well. Notice also that in the case of arrays, ImageInfo is created for each element in the array.
		// So imagine that we have a normal sampler, and a sampler array in the shader which has 10 elements. In this case we'd have:
		// - 2 sampler declarations in the shader, 2 sampler instances and 2 sampler descriptors in MaterialInstance
		// - 11 elements in the mSamplerImages array
		// The two sampler descriptors in the mSamplerDescriptors will contain count and offset info into the mSamplerImages array.
		// 
		// Notice that most of the information in VkWriteDescriptorSets remains constant: everything regarding the shader layout and the bindings do 
		// not change. So we build these write descriptor sets up front and only change the information that can change. These are:
		// - WriteDescriptorSet (which is acquired during update())
		// - What texture is bound (so: image info)
		//  
		for (int sampler_index = 0; sampler_index < sampler_declarations.size(); ++sampler_index)
		{
			const SamplerDeclaration& declaration = sampler_declarations[sampler_index];
			bool is_array = declaration.mNumArrayElements > 1;

			// Check if the sampler is set as override in the MaterialInstance
			const Sampler* sampler = findSamplerResource(getResource()->mSamplers, declaration);
			SamplerInstance* sampler_instance = nullptr;
			if (sampler != nullptr)
			{
				// Sampler is overridden, make an SamplerInstance object
				std::unique_ptr<SamplerInstance> sampler_instance_override;
				if (is_array)
					sampler_instance_override = std::make_unique<Sampler2DArrayInstance>(*mRenderService, declaration, (Sampler2DArray*)sampler, std::bind(&MaterialInstance::onSamplerChanged, this, (int)mSamplerDescriptors.size(), std::placeholders::_1));
				else
					sampler_instance_override = std::make_unique<Sampler2DInstance>(*mRenderService, declaration, (Sampler2D*)sampler, std::bind(&MaterialInstance::onSamplerChanged, this, (int)mSamplerDescriptors.size(), std::placeholders::_1));

				if (!sampler_instance_override->init(errorState))
					return false;

				sampler_instance = sampler_instance_override.get();
				addSamplerInstance(std::move(sampler_instance_override));
			}
			else
			{
				// Sampler is not overridden, find it in the Material
				sampler_instance = material->findSampler(declaration.mName);
			}

			// Store the offset into the mSamplerImages array. This can either be the first index of an array, or just the element itself if it's not
			size_t sampler_descriptor_start_index = mSamplerDescriptors.size();
			VkSampler vk_sampler = sampler_instance->getVulkanSampler();
			if (is_array)
			{
				// Create all VkDescriptorImageInfo for all elements in the array
				Sampler2DArrayInstance* sampler_2d_array = (Sampler2DArrayInstance*)(sampler_instance);

				for (int index = 0; index < sampler_2d_array->getNumElements(); ++index)
				{
					if (sampler_2d_array->hasTexture(index))
						addImageInfo(sampler_2d_array->getTexture(index), vk_sampler);
					else
						addImageInfo(emptyTexture, vk_sampler);
				}
			}
			else
			{
				// Create a single VkDescriptorImageInfo for just this element
				Sampler2DInstance* sampler_2d = (Sampler2DInstance*)(sampler_instance);

				if (sampler_2d->hasTexture())
					addImageInfo(sampler_2d->getTexture(), vk_sampler);
				else
					addImageInfo(emptyTexture, vk_sampler);
			}

			// Create the write descriptor set. This set points to either a single element for non-arrays, or a list of contiguous elements for arrays.
			VkWriteDescriptorSet& write_descriptor_set = mSamplerWriteDescriptorSets[sampler_index];
			write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_descriptor_set.dstSet = VK_NULL_HANDLE;
			write_descriptor_set.dstBinding = sampler_instance->getDeclaration().mBinding;
			write_descriptor_set.dstArrayElement = 0;
			write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_descriptor_set.descriptorCount = mSamplerDescriptors.size() - sampler_descriptor_start_index;
			write_descriptor_set.pImageInfo = mSamplerDescriptors.data() + sampler_descriptor_start_index;
		}

		return true;
	}


	void BaseMaterialInstance::updateSamplers(const DescriptorSet& descriptorSet)
	{
		// We acquired 'some' compatible DescriptorSet with unknown contents. The dstSet must be overwritten
		// with the actual set that was acquired.
		// The actual latest images were already set correctly in mSamplerDescriptors during init and when setting
		// a new texture for a sampler. We just need to call VkUpdateDescriptors with the correct descriptorSet and
		// latest image info.
		for (VkWriteDescriptorSet& write_descriptor : mSamplerWriteDescriptorSets)
			write_descriptor.dstSet = descriptorSet.mSet;

		vkUpdateDescriptorSets(mDevice, mSamplerWriteDescriptorSets.size(), mSamplerWriteDescriptorSets.data(), 0, nullptr);
	}


	void BaseMaterialInstance::updateBindings(const DescriptorSet& descriptorSet)
	{
		// We acquired 'some' compatible DescriptorSet with unknown contents. The dstSet must be overwritten
		// with the actual set that was acquired.
		// The actual latest buffers were already set correctly in mStorageDescriptors during init and when setting
		// a new texture for a sampler. We just need to call VkUpdateDescriptors with the correct descriptorSet and
		// latest buffer info.
		for (VkWriteDescriptorSet& write_descriptor : mStorageWriteDescriptorSets)
			write_descriptor.dstSet = descriptorSet.mSet;

		vkUpdateDescriptorSets(mDevice, mStorageWriteDescriptorSets.size(), mStorageWriteDescriptorSets.data(), 0, nullptr);
	}


	bool BaseMaterialInstance::initInternal(RenderService& renderService, utility::ErrorState& errorState)
	{
		mDevice = renderService.getDevice();
		mRenderService = &renderService;

		BaseMaterial* material = getBaseMaterial();
		const BaseShader* shader = material->getBaseShader();

		// Here we create UBOs in two parts:
		// 1) We create a hierarchical uniform instance structure based on the hierarchical declaration structure from the shader. We do
		//    this only for the properties in the MaterialInstance (so: the properties that were overridden). We've also already done this
		//    in the Material, so after this pass we have a hierarchical structure in Material for all properties, and a similar structure
		//    for the MaterialInstance, but only for the properties that we've overridden.
		// 2) After pass 1, we create the UBO, which is a non-hierarchical structure that holds pointers to all leaf elements. These leaf
		//    elements can point to either Material or MaterialInstance instance uniforms, depending on whether the property was overridden.
		//    Notice that this also means that this structure should be rebuilt when a 'new' override is made at runtime. This is handled in
		//    update() by rebuilding the UBO when a new uniform is created.
		const std::vector<BufferObjectDeclaration>& ubo_declarations = shader->getUBODeclarations();
		for (const BufferObjectDeclaration& ubo_declaration : ubo_declarations)
		{
			const UniformStruct* struct_resource = rtti_cast<const UniformStruct>(findUniformStructMember(getResource()->mUniforms, ubo_declaration));

			// Pass 1: create hierarchical structure
			UniformStructInstance* override_struct = nullptr;
			if (struct_resource != nullptr)
			{
				override_struct = &createUniformRootStruct(ubo_declaration, std::bind(&BaseMaterialInstance::onUniformCreated, this));
				if (!override_struct->addUniformRecursive(ubo_declaration, struct_resource, std::bind(&BaseMaterialInstance::onUniformCreated, this), false, errorState))
					return false;
			}

			// Verify buffer object type
			if (!errorState.check(ubo_declaration.mDescriptorType == EDescriptorType::Uniform, utility::stringFormat("Buffer Object Type mismatch in shader declaration %s", ubo_declaration.mName.c_str())))
				return false;

			// Pass 2: gather leaf uniform instances for a single ubo
			UniformBufferObject ubo(ubo_declaration);
			rebuildUBO(ubo, override_struct);

			mUniformBufferObjects.emplace_back(std::move(ubo));
		}
		mUniformsCreated = false;

		if (!initBindings(errorState))
			return false;

		if (!initSamplers(errorState))
			return false;

		// We get/create an allocator that is compatible with the layout of the shader that this material is bound to. Practically this
		// means a descriptor with:
		// - Same number of UBOs and samplers
		// - Same layout bindings
		// So, any MaterialInstance that is bound to the same shader will be able to allocate from the same DescriptorSetAllocator. It is even
		// possible that multiple shaders that have the same bindings, number of UBOs and samplers can share the same allocator. This is advantageous
		// because internally, pools are created that are allocated from. We want as little empty space in those pools as possible (we want the allocators
		// to act as 'globally' as possible).
		mDescriptorSetCache = &renderService.getOrCreateDescriptorSetCache(shader->getDescriptorSetLayout());

		return true;
	}


	const DescriptorSet& BaseMaterialInstance::update()
	{
		// The UBO contains pointers to all leaf uniform instances. These can be either defines in the material or the 
		// material instance, depending on whether it's overridden. If new overrides were created between update calls,
		// we need to patch pointers in the UBO structure to make sure they point to the correct instances.
		if (mUniformsCreated)
		{
			for (UniformBufferObject& ubo : mUniformBufferObjects)
				rebuildUBO(ubo, findUniform(ubo.mDeclaration->mName));

			mUniformsCreated = false;
		}

		// The DescriptorSet contains information about all UBOs and samplers, along with the buffers that are bound to it.
		// We acquire a descriptor set that is compatible with our shader. The allocator holds a number of allocated descriptor
		// sets and we acquire one that is not in use anymore (that is not in any active command buffer). We cannot make assumptions
		// about the contents of the descriptor sets. The UBO buffers that are bound to it may have different contents than our
		// MaterialInstance, and the samplers may be bound to different images than those that are currently set in the MaterialInstance.
		// For this reason, we always fully update uniforms and samplers to make the descriptor set up to date with the MaterialInstance
		// contents.
		// The reason why we cannot make any assumptions about the contents of DescriptorSets in the cache is that we can perform multiple
		// updates & draws of a MaterialInstance within a single frame. How many draws we do for a MaterialInstance is unknown, that is 
		// up to the client. Because the MaterialInstance state changes *during* a frame for an unknown amount of draws, we 
		// cannot associate DescriptorSet state as returned from the allocator with the latest MaterialInstance state. One way of looking
		// at it is that MaterialInstance's state is 'volatile'. This means we cannot perform dirty checking.
		// One way to tackle this is by maintaining a hash for the uniform/sampler constants that is maintained both in the allocator for
		// a descriptor set and in MaterialInstance. We could then prefer to acquire descriptor sets that have matching hashes.
		const DescriptorSet& descriptor_set = mDescriptorSetCache->acquire(mUniformBufferObjects, mStorageDescriptors.size(), mSamplerDescriptors.size());

		updateUniforms(descriptor_set, mUniformBufferObjects);
		updateBindings(descriptor_set);
		updateSamplers(descriptor_set);

		return descriptor_set;
	}


	//////////////////////////////////////////////////////////////////////////
	// MaterialInstance
	//////////////////////////////////////////////////////////////////////////

	bool MaterialInstance::init(RenderService& renderService, MaterialInstanceResource& resource, utility::ErrorState& errorState)
	{
		mResource = &resource;
		if (!initInternal(renderService, errorState))
			return false;

		return true;
	}


	Material& MaterialInstance::getMaterial()
	{
		return *mResource->mMaterial;
	}


	const nap::Material& MaterialInstance::getMaterial() const
	{
		return *mResource->mMaterial;
	}


	BaseMaterial* MaterialInstance::getBaseMaterial()
	{
		return mResource->mMaterial.get();
	};


	const BaseMaterial* MaterialInstance::getBaseMaterial() const
	{
		return mResource->mMaterial.get();
	};


	const BaseMaterialInstanceResource* MaterialInstance::getResource() const
	{
		return mResource;
	}


	EBlendMode MaterialInstance::getBlendMode() const
	{
		if (mResource->mBlendMode != EBlendMode::NotSet)
			return mResource->mBlendMode;

		return mResource->mMaterial->getBlendMode();
	}


	void MaterialInstance::setBlendMode(EBlendMode blendMode)
	{
		mResource->mBlendMode = blendMode;
	}


	void MaterialInstance::setDepthMode(EDepthMode depthMode)
	{
		mResource->mDepthMode = depthMode;
	}


	EDepthMode MaterialInstance::getDepthMode() const
	{
		if (mResource->mDepthMode != EDepthMode::NotSet)
			return mResource->mDepthMode;

		return mResource->mMaterial->getDepthMode();
	}


	//////////////////////////////////////////////////////////////////////////
	// ComputeMaterialInstance
	//////////////////////////////////////////////////////////////////////////

	bool ComputeMaterialInstance::init(RenderService& renderService, ComputeMaterialInstanceResource& resource, utility::ErrorState& errorState)
	{
		mResource = &resource;
		if (!initInternal(renderService, errorState))
			return false;

		return true;
	}


	ComputeMaterial& ComputeMaterialInstance::getComputeMaterial()
	{
		return *mResource->mComputeMaterial;
	}


	const ComputeMaterial& ComputeMaterialInstance::getComputeMaterial() const
	{
		return *mResource->mComputeMaterial;
	}


	BaseMaterial* ComputeMaterialInstance::getBaseMaterial()
	{
		return mResource->mComputeMaterial.get();
	};


	const BaseMaterial* ComputeMaterialInstance::getBaseMaterial() const
	{
		return mResource->mComputeMaterial.get();
	};


	const BaseMaterialInstanceResource* ComputeMaterialInstance::getResource() const
	{
		return mResource;
	}
}
