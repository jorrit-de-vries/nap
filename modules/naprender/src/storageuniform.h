/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// Local Includes
#include "uniformdeclarations.h"
#include "gpuvaluebuffer.h"
#include "gpustructbuffer.h"

// External Includes
#include <rtti/objectptr.h>
#include <glm/glm.hpp>
#include <utility/dllexport.h>
#include <nap/resource.h>

namespace nap
{
	// Forward Declares
	class StorageUniformInstance;
	class StorageUniformBuffer;

	using StorageUniformCreatedCallback = std::function<void()>;

	/**
	 * Shader storage uniform resource base class.
	 */
	class NAPAPI StorageUniform : public Resource
	{
		RTTI_ENABLE(Resource)
	public:
		std::string mName;		///< Name of uniform in shader
	};


	/**
	 * Storage Uniform buffer container
	 */
	class NAPAPI StorageUniformStruct : public StorageUniform
	{
		RTTI_ENABLE(StorageUniform)
	public:
		rtti::ObjectPtr<StorageUniformBuffer> mStorageUniformBuffer;
	};


	/**
	 * Storage uniform buffer base class
	 */
	class NAPAPI StorageUniformBuffer : public StorageUniform
	{
		RTTI_ENABLE(StorageUniform)
	public:
		/**
		 * @return The number of elements in this array
		 */
		virtual int getCount() const = 0;

		/**
		 * @return The size in bytes
		 */
		virtual size_t getSize() const = 0;

		/**
		 * @return Whether a buffer is set
		 */
		virtual bool hasBuffer() const = 0;
	};


	/**
	 * Structured data
	 */
	class NAPAPI StorageUniformValueBuffer : public StorageUniformBuffer
	{
		RTTI_ENABLE(StorageUniformBuffer)
	};


	/**
	 * Structured data
	 */
	template <typename T>
	class NAPAPI TypedStorageUniformValueBuffer : public StorageUniformValueBuffer
	{
		RTTI_ENABLE(StorageUniformValueBuffer)
	public:
		/**
		 * @return total number of elements
		 */
		virtual int getCount() const override { return hasBuffer() ? mBuffer->mCount : 0; }

		/**
		 * @return The size in bytes
		 */
		virtual size_t getSize() const override { return mBuffer->getSize(); }

		virtual bool hasBuffer() const override { return mBuffer != nullptr; }

		rtti::ObjectPtr<TypedGPUValueBuffer<T>> mBuffer = nullptr;	/// Property 'Buffer'
	};


	/**
	 * Block of uniform data
	 */
	class NAPAPI StorageUniformStructBuffer : public StorageUniformBuffer
	{
		RTTI_ENABLE(StorageUniformBuffer)
	public:
		/**
		 * @return total number of elements.
		 */
		virtual int getCount() const override { return mBuffer->getCount(); }

		/**
		 * @return The size in bytes
		 */
		virtual size_t getSize() const override { return mBuffer->getSize(); }

		/**
		 * @return if the buffer is set
		 */
		virtual bool hasBuffer() const override { return mBuffer != nullptr; }

		rtti::ObjectPtr<GPUStructBuffer> mBuffer = nullptr;
	};


	/**
	 * Find a shader uniform based on the given shader uniform declaration.
	 * @param members uniforms of type nap::Uniform to search through.
	 * @param declaration uniform declaration to match
	 * @return uniform that matches with the given shader declaration, nullptr if not found.
	 */
	template<class T>
	const StorageUniform* findStorageUniformStructMember(const std::vector<T>& members, const ShaderVariableDeclaration& declaration)
	{
		for (auto& member : members)
			if (member->mName == declaration.mName)
				return member.get();
		return nullptr;
	}


	//////////////////////////////////////////////////////////////////////////
	// Storage uniform value buffer type definitions
	//////////////////////////////////////////////////////////////////////////

	using StorageUniformIntBuffer = TypedStorageUniformValueBuffer<int>;
	using StorageUniformFloatBuffer = TypedStorageUniformValueBuffer<float>;
	using StorageUniformVec2Buffer = TypedStorageUniformValueBuffer<glm::vec2>;
	using StorageUniformVec3Buffer = TypedStorageUniformValueBuffer<glm::vec3>;
	using StorageUniformVec4Buffer = TypedStorageUniformValueBuffer<glm::vec4>;
	using StorageUniformMat4Buffer = TypedStorageUniformValueBuffer<glm::mat4>;
}
