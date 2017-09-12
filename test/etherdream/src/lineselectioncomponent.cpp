#include "lineselectioncomponent.h"
#include <mathutils.h>
#include <nap/entity.h>
#include <nap/logger.h>

RTTI_BEGIN_CLASS(nap::LineSelectionComponent)
	RTTI_PROPERTY("Lines", &nap::LineSelectionComponent::mLines, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Index", &nap::LineSelectionComponent::mIndex, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::LineSelectionComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	bool LineSelectionComponentInstance::init(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		// Copy over the list of lines to selection from
		mLines = getComponent<LineSelectionComponent>()->mLines;

		// Ensure there are lines to choose from
		if (!(errorState.check(mLines.size() > 0, "No lines to select from")))
		{
			assert(false);
			return false;
		}

		// Make sure index is in range
		verifyIndex(getComponent<LineSelectionComponent>()->mIndex);

		// Done
		return true;
	}


	const nap::PolyLine& LineSelectionComponentInstance::getLine() const
	{
		return *(mLines[mIndex]);
	}


	nap::PolyLine& LineSelectionComponentInstance::getLine()
	{
		return *(mLines[mIndex]);
	}


	void LineSelectionComponentInstance::setIndex(int index)
	{
		verifyIndex(index);
	}


	void LineSelectionComponentInstance::update(double deltaTime)
	{	}


	void LineSelectionComponentInstance::verifyIndex(int index)
	{
		// Make sure the index is in range
		mIndex = math::clamp<int>(index, 0, mLines.size() - 1);
	}
}