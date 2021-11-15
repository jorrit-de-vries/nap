/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "storageuniforminstance.h"

RTTI_DEFINE_BASE(nap::StorageUniformInstance)
RTTI_DEFINE_BASE(nap::StorageUniformBufferInstance)
RTTI_DEFINE_BASE(nap::StorageUniformValueBufferInstance)

//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformStructInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformStructDeclaration&, const nap::UniformCreatedCallback&)
//	RTTI_FUNCTION("findUniform", (nap::UniformInstance* (nap::UniformStructInstance::*)(const std::string&)) &nap::UniformStructInstance::findUniform)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformStructArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformStructArrayDeclaration&)
//	RTTI_FUNCTION("findElement", (nap::UniformStructInstance* (nap::UniformStructArrayInstance::*)(int)) &nap::UniformStructArrayInstance::findElement)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformIntInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformIntInstance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformFloatInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformFloatInstance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec2Instance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec2Instance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec3Instance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec3Instance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec4Instance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec4Instance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformMat4Instance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformMat4Instance::setValue)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformIntArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformIntArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformIntArrayInstance::getNumElements)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformFloatArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformFloatArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformFloatArrayInstance::getNumElements)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec2ArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec2ArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformVec2ArrayInstance::getNumElements)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec3ArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec3ArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformVec3ArrayInstance::getNumElements)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformVec4ArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformVec4ArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformVec4ArrayInstance::getNumElements)
//RTTI_END_CLASS
//
//RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::UniformMat4ArrayInstance)
//	RTTI_CONSTRUCTOR(const nap::UniformValueArrayDeclaration&)
//	RTTI_FUNCTION("setValue", &nap::UniformMat4ArrayInstance::setValue)
//	RTTI_FUNCTION("getNumElements", &nap::UniformMat4ArrayInstance::getNumElements)
//RTTI_END_CLASS


namespace nap
{
	template<typename INSTANCE_TYPE, typename RESOURCE_TYPE, typename DECLARATION_TYPE>
	static std::unique_ptr<INSTANCE_TYPE> createUniformValueInstance(const StorageUniform* value, const DECLARATION_TYPE& declaration, utility::ErrorState& errorState)
	{
		std::unique_ptr<INSTANCE_TYPE> result = std::make_unique<INSTANCE_TYPE>(declaration);
		if (value != nullptr)
		{
			const RESOURCE_TYPE* typed_resource = rtti_cast<const RESOURCE_TYPE>(value);
			if (!errorState.check(typed_resource != nullptr, "Encountered type mismatch between uniform in material and uniform in shader"))
				return nullptr;

			result->set(*typed_resource);
		}
		return result;
	}


	//////////////////////////////////////////////////////////////////////////
	// UniformStructInstance
	//////////////////////////////////////////////////////////////////////////

	std::unique_ptr<StorageUniformInstance> StorageUniformStructInstance::createUniformFromDeclaration(const UniformDeclaration& declaration, const StorageUniformCreatedCallback& uniformCreatedCallback)
	{
		rtti::TypeInfo declaration_type = declaration.get_type();

		if (declaration_type == RTTI_OF(UniformStructArrayDeclaration))
		{
			const UniformStructArrayDeclaration* struct_array_declaration = rtti_cast<const UniformStructArrayDeclaration>(&declaration);
			std::unique_ptr<StorageUniformDataBufferInstance> buffer_instance = std::make_unique<StorageUniformDataBufferInstance>(*struct_array_declaration);
			for (auto& struct_declaration : struct_array_declaration->mElements)
			{
				//std::unique_ptr<UniformStructInstance> struct_instance = std::make_unique<UniformStructInstance>(*struct_declaration, uniformCreatedCallback);
				//struct_array_instance->addElement(std::move(struct_instance));
			}
			return std::move(buffer_instance);
		}
		else if (declaration_type == RTTI_OF(UniformValueArrayDeclaration))
		{
			const UniformValueArrayDeclaration* value_array_declaration = rtti_cast<const UniformValueArrayDeclaration>(&declaration);

			if (value_array_declaration->mElementType == EUniformValueType::Int)
			{
				std::unique_ptr<StorageUniformIntBufferInstance> buffer_instance = std::make_unique<StorageUniformIntBufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
			else if (value_array_declaration->mElementType == EUniformValueType::Float)
			{
				std::unique_ptr<StorageUniformFloatBufferInstance> buffer_instance = std::make_unique<StorageUniformFloatBufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
			else if (value_array_declaration->mElementType == EUniformValueType::Vec2)
			{
				std::unique_ptr<StorageUniformVec2BufferInstance> buffer_instance = std::make_unique<StorageUniformVec2BufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
			else if (value_array_declaration->mElementType == EUniformValueType::Vec3)
			{
				std::unique_ptr<StorageUniformVec3BufferInstance> buffer_instance = std::make_unique<StorageUniformVec3BufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
			else if (value_array_declaration->mElementType == EUniformValueType::Vec4)
			{
				std::unique_ptr<StorageUniformVec4BufferInstance> buffer_instance = std::make_unique<StorageUniformVec4BufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
			else if (value_array_declaration->mElementType == EUniformValueType::Mat4)
			{
				std::unique_ptr<StorageUniformMat4BufferInstance> buffer_instance = std::make_unique<StorageUniformMat4BufferInstance>(*value_array_declaration);
				return std::move(buffer_instance);
			}
		}
		else if (declaration_type == RTTI_OF(UniformStructDeclaration))
		{
			const UniformStructDeclaration* struct_declaration = rtti_cast<const UniformStructDeclaration>(&declaration);
			return std::make_unique<StorageUniformStructInstance>(*struct_declaration, uniformCreatedCallback);
		}
		else
		{
			const UniformValueDeclaration* value_declaration = rtti_cast<const UniformValueDeclaration>(&declaration);

			//if (value_declaration->mType == EUniformValueType::Int)
			//{
			//	return std::make_unique<StorageUniformIntInstance>(*value_declaration);
			//}
			//else if (value_declaration->mType == EUniformValueType::Float)
			//{
			//	return std::make_unique<UniformFloatInstance>(*value_declaration);
			//}
			//else if (value_declaration->mType == EUniformValueType::Vec2)
			//{
			//	return std::make_unique<UniformVec2Instance>(*value_declaration);
			//}
			//else if (value_declaration->mType == EUniformValueType::Vec3)
			//{
			//	return std::make_unique<UniformVec3Instance>(*value_declaration);
			//}
			//else if (value_declaration->mType == EUniformValueType::Vec4)
			//{
			//	return std::make_unique<UniformVec4Instance>(*value_declaration);
			//}
			//else if (value_declaration->mType == EUniformValueType::Mat4)
			//{
			//	return std::make_unique<UniformMat4Instance>(*value_declaration);
			//}
		}

		return nullptr;
	}


	nap::StorageUniformInstance* StorageUniformStructInstance::findStorageUniform(const std::string& name)
	{
		for (auto& uniform_instance : mUniforms)
		{
			if (uniform_instance->getDeclaration().mName == name)
				return uniform_instance.get();
		}
		return nullptr;
	}


	bool StorageUniformStructInstance::addStorageUniform(const UniformStructDeclaration& structDeclaration, const StorageUniformStruct* structResource, const StorageUniformChangedCallback& uniformChangedCallback, bool createDefaults, utility::ErrorState& errorState)
	{
		for (auto& uniform_declaration : structDeclaration.mMembers)
		{
			rtti::TypeInfo declaration_type = uniform_declaration->get_type();

			// TODO: Implement recursive structures - StorageUniformStructs currently only hold a single StorageUniformBuffer
			const StorageUniform* resource = nullptr;
			if (structResource != nullptr)
				if (uniform_declaration->mName == structResource->mStorageUniformBuffer->mName)
					resource = structResource->mStorageUniformBuffer.get();

			if (resource == nullptr)
				continue;

			if (declaration_type == RTTI_OF(UniformStructArrayDeclaration))
			{
				UniformStructArrayDeclaration* struct_array_declaration = rtti_cast<UniformStructArrayDeclaration>(uniform_declaration.get());

				//std::unique_ptr<StorageUniformDataBuffer> data_buffer_instance = std::make_unique<StorageUniformDataBuffer>(*struct_array_declaration);
				//const StorageUniformDataBuffer* data_buffer_resource = rtti_cast<const StorageUniformDataBuffer>(resource);

				// TODO: Create buffer & determine the buffer size here? Or use some sort of comparable buffer descriptor?
				//mUniforms.emplace_back(std::move(data_buffer_instance));

				// TODO: Remove
				errorState.fail("Nested storage uniform structs are not yet supported");
				return false;
			}
			else if (declaration_type == RTTI_OF(UniformValueArrayDeclaration))
			{
				const StorageUniformValueBuffer* value_buffer_resource = rtti_cast<const StorageUniformValueBuffer>(resource);
				if (!errorState.check(resource == nullptr || value_buffer_resource != nullptr, "Type mismatch between shader type and json type"))
					return false;

				UniformValueArrayDeclaration* value_declaration = rtti_cast<UniformValueArrayDeclaration>(uniform_declaration.get());
				std::unique_ptr<StorageUniformValueBufferInstance> instance_value_buffer;

				if (value_declaration->mElementType == EUniformValueType::Int)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformIntBufferInstance, StorageUniformIntBuffer>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mElementType == EUniformValueType::Float)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformFloatBufferInstance, StorageUniformFloatBuffer>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mElementType == EUniformValueType::Vec2)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformVec2BufferInstance, StorageUniformVec2Buffer>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mElementType == EUniformValueType::Vec3)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformVec3BufferInstance, StorageUniformVec3Buffer>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mElementType == EUniformValueType::Vec4)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformVec4BufferInstance, StorageUniformVec4Buffer>(resource, *value_declaration, errorState);
				}
				else if (value_declaration->mElementType == EUniformValueType::Mat4)
				{
					instance_value_buffer = createUniformValueInstance<StorageUniformMat4BufferInstance, StorageUniformMat4Buffer>(resource, *value_declaration, errorState);
				}
				else
				{
					assert(false);
				}

				if (instance_value_buffer == nullptr)
					return false;

				// If the array was not set in json, we need to ensure the array has the correct size & is filled with default values
				//if (resource == nullptr)
				//	instance_value_buffer->setDefault();

				if (!errorState.check(resource == nullptr || value_buffer_resource->getCount() == value_declaration->mNumElements, "Encountered mismatch in array elements between array in material and array in shader"))
					return false;

				mUniforms.emplace_back(std::move(instance_value_buffer));
			}
			else if (declaration_type == RTTI_OF(UniformStructDeclaration))
			{
				UniformStructDeclaration* struct_declaration = rtti_cast<UniformStructDeclaration>(uniform_declaration.get());

				//std::unique_ptr<StorageUniformDataBuffer> data_buffer_instance = std::make_unique<StorageUniformDataBuffer>(*struct_declaration);
				//const StorageUniformDataBuffer* data_buffer_resource = rtti_cast<const StorageUniformDataBuffer>(resource);

				// TODO: Create buffer & determine the buffer size here? Or use some sort of comparable buffer descriptor?
				//if (!data_buffer_instance->addUniformRecursive(*struct_declaration, data_buffer_resource, uniformCreatedCallback, createDefaults, errorState))
				//	return false;

				//mUniforms.emplace_back(std::move(data_buffer_instance));

				// TODO: Remove
				errorState.fail("Nested storage uniform structs are not yet supported");
				return false;
			}
			else
			{
				//UniformValueDeclaration* value_declaration = rtti_cast<UniformValueDeclaration>(uniform_declaration.get());
				//std::unique_ptr<StorageUniformValueInstance> value_instance;

				//if (value_declaration->mType == EUniformValueType::Int)
				//{
				//	value_instance = createUniformValueInstance<UniformIntInstance, UniformInt>(resource, *value_declaration, errorState);
				//}
				//else if (value_declaration->mType == EUniformValueType::Float)
				//{
				//	value_instance = createUniformValueInstance<UniformFloatInstance, UniformFloat>(resource, *value_declaration, errorState);
				//}
				//else if (value_declaration->mType == EUniformValueType::Vec2)
				//{
				//	value_instance = createUniformValueInstance<UniformVec2Instance, UniformVec2>(resource, *value_declaration, errorState);
				//}
				//else if (value_declaration->mType == EUniformValueType::Vec3)
				//{
				//	value_instance = createUniformValueInstance<UniformVec3Instance, UniformVec3>(resource, *value_declaration, errorState);
				//}
				//else if (value_declaration->mType == EUniformValueType::Vec4)
				//{
				//	value_instance = createUniformValueInstance<UniformVec4Instance, UniformVec4>(resource, *value_declaration, errorState);
				//}
				//else if (value_declaration->mType == EUniformValueType::Mat4)
				//{
				//	value_instance = createUniformValueInstance<UniformMat4Instance, UniformMat4>(resource, *value_declaration, errorState);
				//}
				//else
				//{
				//	assert(false);
				//}

				//if (value_instance == nullptr)
				//	return false;

				//mUniforms.emplace_back(std::move(value_instance));

				// TODO: Remove
				errorState.fail("Storage uniform values are not yet supported");
				return false;
			}
		}
		return true;
	}
}
