#include "osclaserinputhandler.h"

#include <nap/entity.h>
#include <polyline.h>
#include <utility/stringutils.h>
#include <mathutils.h>

RTTI_BEGIN_CLASS(nap::OSCLaserInputHandler)
	RTTI_PROPERTY("SelectionComponentOne", &nap::OSCLaserInputHandler::mSelectionComponentOne, nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("SelectionComponentTwo", &nap::OSCLaserInputHandler::mSelectionComponentTwo, nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::OSCLaserInputHandlerInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////

namespace nap
{
	OSCLaserInputHandlerInstance::~OSCLaserInputHandlerInstance()
	{
		mInputComponent->messageReceived.disconnect(mMessageReceivedSlot);
	}


	bool OSCLaserInputHandlerInstance::init(EntityCreationParameters& entityCreationParams, utility::ErrorState& errorState)
	{
		mRotateComponent = getEntityInstance()->findComponent<nap::RotateComponentInstance>();
		if (!errorState.check(mRotateComponent != nullptr, "missing rotate component"))
			return false;

		mInputComponent = getEntityInstance()->findComponent<nap::OSCInputComponentInstance>();
		if (!errorState.check(mInputComponent != nullptr, "missing osc input component"))
			return false;

		ComponentInstance* selection_one = getComponent<OSCLaserInputHandler>()->mSelectionComponentOne.get();
		ComponentInstance* selection_two = getComponent<OSCLaserInputHandler>()->mSelectionComponentTwo.get();

		if (!(selection_one->get_type().is_derived_from(RTTI_OF(LineSelectionComponentInstance))))
			return errorState.check(false, "selection component one is not a line selection component");

		if (!(selection_two->get_type().is_derived_from(RTTI_OF(LineSelectionComponentInstance))))
			return errorState.check(false, "selection component two is not a line selection component");

		// Set the two selector objects
		mSelectorOne = static_cast<LineSelectionComponentInstance*>(selection_one);
		mSelectorTwo = static_cast<LineSelectionComponentInstance*>(selection_two);

		mInputComponent->messageReceived.connect(mMessageReceivedSlot);


		return true;
	}


	void OSCLaserInputHandlerInstance::handleMessageReceived(const nap::OSCEvent& oscEvent)
	{
		if (utility::gStartsWith(oscEvent.getAddress(), "/color"))
		{
			// Get values
			std::vector<std::string> out_values;
			utility::gSplitString(oscEvent.getAddress(), '/', out_values);
			
			// Get index (of selection component)
			assert(out_values.size() == 4);
			int index = math::clamp<int>(std::stoi(out_values[2]) - 1, 0, 1);

			// Get color channel 
			int channel = std::stoi(out_values.back()) - 1;
			assert(channel <= 4 && channel >= 0);

			// Update color
			updateColor(oscEvent, index, channel);
			return;
		}

		if (utility::gStartsWith(oscEvent.getAddress(), "/rotation"))
		{
			updateRotate(oscEvent);
			return;
		}

		if (utility::gStartsWith(oscEvent.getAddress(), "/resetrotation"))
		{
			resetRotate(oscEvent);
			return;
		}

		if (utility::gStartsWith(oscEvent.getAddress(), "/selection"))
		{
			// Get index
			std::vector<std::string> out_values;
			utility::gSplitString(oscEvent.getAddress(), '/', out_values);

			assert(out_values.size() == 3);
			int index = math::clamp<int>(std::stoi(out_values.back())-1, 0,1);
			setIndex(oscEvent, index);
			return;
		}
	}


	void OSCLaserInputHandlerInstance::updateColor(const OSCEvent& oscEvent, int index, int channel)
	{
		// New value
		assert(oscEvent[0].isFloat());
		float v = oscEvent[0].asFloat();

		// Get line to update
		LineSelectionComponentInstance* selector = index == 0 ? mSelectorOne : mSelectorTwo;

		// Get the vertex colors
		nap::PolyLine& mesh = selector->getLine();
		Vec4VertexAttribute& color_attr = mesh.getColorAttr();

		// Get current color based on first vertex (dirty but ok for now)
		assert(color_attr.getSize() > 0);
		glm::vec4 ccolor = color_attr.getData()[0];

		// Set color to be new value
		ccolor[channel] = v;

        std::vector<glm::vec4> new_color(color_attr.getSize(), ccolor);
		color_attr.setData(new_color);
		nap::utility::ErrorState error;
		if (!(mesh.getMeshInstance().update(error)))
		{
			assert(false);
		}
	}


	void OSCLaserInputHandlerInstance::updateRotate(const OSCEvent& oscEvent)
	{
		// New value
		assert(oscEvent.getArgument(0).isFloat());
		float v = oscEvent.getArgument(0).asFloat();
		
		// Get index
		std::vector<std::string> parts;
		utility::gSplitString(oscEvent.getAddress(), '/', parts);
		
		// Get last
		int idx = std::stoi(parts.back().c_str());

		switch (idx)
		{
		case 1:
			mRotateComponent->mProperties.mAxis.x = v;
			break;
		case 2:
			mRotateComponent->mProperties.mAxis.y = v;
			break;
		case 3:
			mRotateComponent->mProperties.mAxis.z = v;
			break;
		case 4:
			mRotateComponent->mProperties.mSpeed = v;
			break;
		default:
			assert(false);
		}
	}


	void OSCLaserInputHandlerInstance::resetRotate(const OSCEvent& event)
	{
		assert(event[0].isFloat());
		float v = event[0].asFloat();

		if (v < 0.99f)
			return;

		mRotateComponent->reset();
		mRotateComponent->mProperties.mSpeed = 0.0f;
	}


	void OSCLaserInputHandlerInstance::setIndex(const OSCEvent& event, int index)
	{
		assert(event[0].isFloat());
		float v = event[0].asFloat();

		// Get line to update
		LineSelectionComponentInstance* selector = index == 0 ? mSelectorOne : mSelectorTwo;

		// Map value to range
		float count = static_cast<float>(selector->getCount());
		int idx = math::min<int>(static_cast<int>(count * v), count - 1);
		selector->setIndex(idx);
	}


	void OSCLaserInputHandler::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.emplace_back(RTTI_OF(nap::RotateComponent));
		components.emplace_back(RTTI_OF(nap::OSCInputComponent));
	}

}
