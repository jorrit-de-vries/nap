/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <vector>
#include "uniforminstance.h"
#include "storageuniforminstance.h"
#include "shadervariabledeclarations.h"
#include "gpubuffer.h"

namespace nap
{
	/**
	 * Blend mode for Materials.
	 */
	enum class EBlendMode : int
	{
		NotSet = 0,				///< Default value for MaterialInstances, means that the Material's blend mode is used instead
		Opaque,					///< Regular opaque, similar to (One, Zero) blend
		AlphaBlend,				///< Transparent object (SrcAlpha, InvSrcAlpha) blend
		Additive				///< Additive, (One, One) blend
	};


	enum class ECullWindingOrder : int
	{
		Clockwise,
		CounterClockwise
	};


	/**
	 * Determines how the z-buffer is used for reading and writing.
	 */
	enum class EDepthMode : int
	{
		NotSet = 0,				///< Default value for MaterialInstances, means that the Material's blend is used instead
		InheritFromBlendMode,	///< Transparent objects do not write depth, but do read depth. Opaque objects read and write depth.
		ReadWrite,				///< Read and write depth
		ReadOnly,				///< Only read depth
		WriteOnly,				///< Only write depth
		NoReadWrite				///< Neither read or write depth
	};


	class UniformBufferObject
	{
	public:
		using UniformList = std::vector<const UniformLeafInstance*>;

		UniformBufferObject(const BufferObjectDeclaration& declaration) :
			mDeclaration(&declaration)
		{
			assert(declaration.mDescriptorType == EDescriptorType::Uniform);
		}

		const BufferObjectDeclaration*	mDeclaration;
		UniformList								mUniforms;
	};


	class StorageUniformBufferObject
	{
	public:
		using StorageUniformList = std::vector<const StorageUniformBufferInstance*>;

		StorageUniformBufferObject(const BufferObjectDeclaration& declaration) :
			mDeclaration(&declaration)
		{
			assert(declaration.mDescriptorType == EDescriptorType::Storage);
		}

		const BufferObjectDeclaration*		mDeclaration;
		StorageUniformList							mStorageUniforms;
	};
}
