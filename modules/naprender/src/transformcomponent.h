#pragma once

// External Includes
#include <nap/serviceablecomponent.h>
#include <nap/rttinap.h>
#include <nap/coreattributes.h>

// Local Includes
#include "renderattributes.h"

namespace nap
{
	class TransformComponent : public ServiceableComponent
	{
		RTTI_ENABLE_DERIVED_FROM(ServiceableComponent)
	public:
		// Default constructor
		TransformComponent() = default;
		
		// Attributes
		Attribute<glm::vec3>		translate		{ this,	"Translation",	{ 0.0, 0.0, 0.0 } };		// vector 3 - x, y z
		Attribute<glm::vec4>		rotate			{ this, "Rotation",		{ 0.0, 0.0, 0.0, 1.0 } };	// quaternion - x, y, z, w
		Attribute<glm::vec3>		scale			{ this, "Scale",		{ 1.0, 1.0, 1.0 } };		// vector 3 - x, y, z
		
		// Uniform Scale
		NumericAttribute<float>		uniformScale	{ this, "UniformScale", 1.0f, 0.0f, 1.0f };			// vector 3 - x, y, z

		/**
		 * Constructs and returns a local transform
		 * @return this transform local matrix
		 */
		glm::mat4x4 getLocalTransform() const;
	};

} // nap

RTTI_DECLARE(nap::TransformComponent)