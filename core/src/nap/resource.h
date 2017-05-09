#pragma once

// Local Includes
#include <nap/coreattributes.h>
#include "attributeobject.h"
#include "errorstate.h"

// External Includes
#include <string>

namespace nap
{
	// Forward Declares
	class ResourceManagerService;
	class Core;

	/**
	* Abstract base class for any Asset. Could be a TextureAsset, ModelAsset or AudioAsset for example.
	* WARNING: A resource may only be created through the ResourceManagerService providing a valid resource path
	*/
	class Resource : public AttributeObject
	{
		RTTI_ENABLE(AttributeObject)
	public:
		enum class EFinishMode : uint8
		{
			COMMIT,
			ROLLBACK
		};

		Resource() = default;

		/**
		* 
		*/
		virtual bool init(ErrorState& errorState) = 0;

		/**
		* 
		*/
		virtual void finish(EFinishMode mode) = 0;

		/**
		* @return Human readable string representation of this path
		*/
		virtual const std::string getDisplayName() const = 0;
	};
}
