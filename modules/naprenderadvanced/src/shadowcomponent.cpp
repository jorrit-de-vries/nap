#include "shadowcomponent.h"

// Local includes
#include "lightcomponent.h"

// External Includes
#include <entity.h>

// nap::ShadowComponent run time class definition 
RTTI_BEGIN_CLASS(nap::ShadowComponent)
RTTI_END_CLASS

// nap::ShadowComponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ShadowComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	//////////////////////////////////////////////////////////////////////////
	// Constant
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Static
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// ShadowComponent
	//////////////////////////////////////////////////////////////////////////

	void ShadowComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.emplace_back(RTTI_OF(LightComponent));
	}


	//////////////////////////////////////////////////////////////////////////
	// ShadowComponentInstance
	//////////////////////////////////////////////////////////////////////////

	bool ShadowComponentInstance::init(utility::ErrorState& errorState)
	{
		mResource = getComponent<ShadowComponent>();
		return true;
	}


	void ShadowComponentInstance::update(double deltaTime)
	{

	}
}
