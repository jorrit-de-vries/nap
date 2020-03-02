#include "parameterblendcomponent.h"
#include "parameterblender.h"

// External Includes
#include <entity.h>
#include <nap/core.h>
#include <mathutils.h>
#include <rtti/jsonreader.h>
#include <rtti/defaultlinkresolver.h>
#include <nap/logger.h>

// nap::blendparameterscomponent run time class definition 
RTTI_BEGIN_CLASS(nap::ParameterBlendComponent)
	RTTI_PROPERTY("EnableBlending",		&nap::ParameterBlendComponent::mEnableBlending,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("Parameters",			&nap::ParameterBlendComponent::mBlendParameters,	nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("PresetIndex",		&nap::ParameterBlendComponent::mPresetIndex,		nap::rtti::EPropertyMetaData::Required)
	RTTI_PROPERTY("PresetBlendTime",	&nap::ParameterBlendComponent::mPresetBlendTime,	nap::rtti::EPropertyMetaData::Required)
RTTI_END_CLASS

// nap::blendparameterscomponentInstance run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ParameterBlendComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	void ParameterBlendComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{

	}


	ParameterBlendComponentInstance::~ParameterBlendComponentInstance()
	{
		mBlenders.clear();
		mPresets.clear();
		mPresetGroups.clear();
		mPresetData.clear();
	}


	bool ParameterBlendComponentInstance::init(utility::ErrorState& errorState)
	{
		// Get parameter service
		mParameterService = getEntityInstance()->getCore()->getService<ParameterService>();
		assert(mParameterService != nullptr);

		// Get resource
		ParameterBlendComponent* resource = getComponent<ParameterBlendComponent>();

		// Copy resources
		mPresetIndex = resource->mPresetIndex.get();
		mPresetBlendTime = resource->mPresetBlendTime.get();
		mBlendParameters = resource->mBlendParameters.get();
		mEnableBlending = resource->mEnableBlending;

		// Source presets
		if (!sourcePresets(errorState))
			return false;

		// Create blenders
		if (!createBlenders(errorState))
			return false;

		// Called when the preset index changes
		mPresetIndex->valueChanged.connect(mIndexChangedSlot);
		changePreset(mPresetIndex->mValue);

		return true;
	}


	void ParameterBlendComponentInstance::update(double deltaTime)
	{
		// Don't do anything when blending is disabled
		if (!mEnableBlending || !mBlending)
			return;

		// Compute elapsed time value
		mElapsedTime += deltaTime;
		float lerpv = math::smoothStep<float>(getBlendValue(), 0.0f, 1.0f);

		// Update blenders
		for (auto& blender : mBlenders)
		{
			if(!(blender->hasTarget()))
				continue;
			blender->blend(lerpv);
		}

		if (mElapsedTime >= mPresetBlendTime->mValue)
		{
			mElapsedTime = 0.0;
			mBlending = false;
		}
	}


	void ParameterBlendComponentInstance::enable(bool value)
	{
		mEnableBlending = value;
	}


	bool ParameterBlendComponentInstance::isBlending() const
	{
		return mBlending && mEnableBlending;
	}


	float ParameterBlendComponentInstance::getBlendValue()
	{
		return math::fit<float>(mElapsedTime, 0.0f, mPresetBlendTime->mValue, 0.0f, 1.0f);
	}


	bool ParameterBlendComponentInstance::reload(nap::utility::ErrorState& error)
	{
		return sourcePresets(error);
	}


	bool ParameterBlendComponentInstance::sourcePresets(utility::ErrorState& error)
	{
		std::vector<std::string> presets = mParameterService->getPresets(*(mBlendParameters->mParameterGroup));

		mPresetGroups.clear();
		mPresetGroups.reserve(presets.size());

		mPresetData.clear();
		mPresetData.reserve(presets.size());

		mPresets.clear();
		mPresets.reserve(presets.size());
			
		// Load all presets
		nap::rtti::Factory& factory = getEntityInstance()->getCore()->getResourceManager()->getFactory();
		for (const auto& preset : presets)
		{
			// Get path to preset
			std::string preset_path = mParameterService->getPresetPath(mBlendParameters->mParameterGroup->mID, preset);

			// Create object that will hold de-serialization results
			std::unique_ptr<rtti::DeserializeResult> deserialize_result = std::make_unique<rtti::DeserializeResult>();

			// Load the parameters from the preset
			if (!rtti::readJSONFile(preset_path, rtti::EPropertyValidationMode::DisallowMissingProperties, rtti::EPointerPropertyMode::NoRawPointers, factory, *deserialize_result, error))
				return false;

			// Resolve links
			if (!rtti::DefaultLinkResolver::sResolveLinks(deserialize_result->mReadObjects, deserialize_result->mUnresolvedPointers, error))
				return false;

			// Find root group in read objects, the preset needs to have a matching group
			bool preset_group_found = false;
			for (auto& object : deserialize_result->mReadObjects)
			{
				if (object->get_type().is_derived_from<ParameterGroup>() && object->mID == mBlendParameters->mParameterGroup->mID)
				{
					// Valid entry, add as valid preset
					mPresetData.emplace_back(std::move(deserialize_result));
					mPresetGroups.emplace_back(rtti_cast<ParameterGroup>(object.get()));
					mPresets.emplace_back(preset);
					preset_group_found = true;
					break;
				}
			}

			// All good, preset added
			if (preset_group_found)
				continue;

			nap::Logger::warn("%s: No parameter group with id: %s found in preset: %s",
				getComponent<ParameterBlendComponent>()->mID.c_str(),
				mBlendParameters->mParameterGroup->mID.c_str(), preset.c_str());
		}

		// Update range
		mPresetIndex->setRange(0, math::max<int>(mPresets.size() - 1, 0));
		return true;
	}


	bool ParameterBlendComponentInstance::createBlenders(nap::utility::ErrorState& error)
	{
		mBlenders.clear();
		mBlenders.reserve(mBlendParameters->mParameters.size());

		for (auto& source_parameter : mBlendParameters->mParameters)
		{
			// Create new blender
			std::unique_ptr<BaseParameterBlender> new_blender = std::move(getParameterBlender(*source_parameter));
			if (new_blender == nullptr)
			{
				error.fail("%s: Parameter %s can't be blended, no blender available for: %s", 
					getComponent<ParameterBlendComponent>()->mID.c_str(),
					source_parameter->mID.c_str(),
					source_parameter->get_type().get_name().to_string().c_str());
				return false;
			}
			mBlenders.emplace_back(std::move(new_blender));
		}
		return true;
	}


	void ParameterBlendComponentInstance::changePreset(int index)
	{
		// Nothing to update without presets
		if (mPresetGroups.empty())
			return;

		// Get de-serialized result for preset
		assert(index < mPresets.size());
		ParameterGroup& preset_group = *(mPresetGroups[index]);

		// Now update every blender, where for every blender a matching target is found
		for (auto& blender : mBlenders)
		{
			// Get source parameter
			const nap::Parameter& source_param = blender->getParameter();

			// Find matching target parameter in preset
			ResourcePtr<Parameter> found_param = preset_group.findParameterRecursive(source_param.mID);
			
			// If no parameter with a matching id is found, notify and clear
			// This ensures the blender is not updated
			if (found_param == nullptr)
			{
				nap::Logger::warn("%s: Unable to find parameter with id: %s in preset: %s",
					getComponent<ParameterBlendComponent>()->mID.c_str(),
					source_param.mID.c_str(), mPresets[index].c_str());
				blender->clearTarget();
				continue;
			}

			// Set target
			blender->setTarget(found_param.get());
		}

		mElapsedTime = 0.0;
		mBlending = true;
	}
}