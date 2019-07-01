// Local Includes
#include "orthocameracomponent.h"

// External Includes
#include <glm/gtc/matrix_transform.hpp> 
#include "transformcomponent.h"
#include <entity.h>

RTTI_BEGIN_ENUM(nap::EOrthoCameraMode)
	RTTI_ENUM_VALUE(nap::EOrthoCameraMode::PixelSpace,			"PixelSpace"),
	RTTI_ENUM_VALUE(nap::EOrthoCameraMode::CorrectAspectRatio,	"CorrectAspectRatio"),
	RTTI_ENUM_VALUE(nap::EOrthoCameraMode::Custom,				"Custom")
RTTI_END_ENUM

RTTI_BEGIN_CLASS(nap::OrthoCameraProperties)
	RTTI_PROPERTY("Mode",				&nap::OrthoCameraProperties::mMode,					nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("LeftPlane",			&nap::OrthoCameraProperties::mLeftPlane,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("RightPlane",			&nap::OrthoCameraProperties::mRightPlane,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("TopPlane",			&nap::OrthoCameraProperties::mTopPlane,				nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("BottomPlane",		&nap::OrthoCameraProperties::mBottomPlane,			nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("NearClippingPlane",	&nap::OrthoCameraProperties::mNearClippingPlane,	nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("FarClippingPlane",	&nap::OrthoCameraProperties::mFarClippingPlane,		nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::OrthoCameraComponent)
	RTTI_PROPERTY("Properties",			&nap::OrthoCameraComponent::mProperties,	nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::OrthoCameraComponentInstance)
	RTTI_CONSTRUCTOR(nap::EntityInstance&, nap::Component&)
RTTI_END_CLASS

namespace nap
{
	// Hook up attribute changes
	OrthoCameraComponentInstance::OrthoCameraComponentInstance(EntityInstance& entity, Component& resource) :
		CameraComponentInstance(entity, resource)
	{
	}


	bool OrthoCameraComponentInstance::init(utility::ErrorState& errorState)
	{
		mProperties = getComponent<OrthoCameraComponent>()->mProperties;
		mTransformComponent =	getEntityInstance()->findComponent<TransformComponentInstance>();
		if (!errorState.check(mTransformComponent != nullptr, "%s: missing transform component", mID.c_str()))
			return false;

		return true;
	}


	void OrthoCameraComponentInstance::setRenderTargetSize(const glm::ivec2& size)
	{
		if (size != getRenderTargetSize())
		{
			CameraComponentInstance::setRenderTargetSize(size);
			setDirty();
		}
	}


	void OrthoCameraComponentInstance::setProperties(const OrthoCameraProperties& properties)
	{
		mProperties = properties;
		setDirty();
	}


	void OrthoCameraComponentInstance::setMode(EOrthoCameraMode mode)
	{
		if(mProperties.mMode != mode)
		{
			mProperties.mMode = mode;
			setDirty();
		}
	}


	// Computes projection matrix if dirty, otherwise returns the
	// cached version
	const glm::mat4& OrthoCameraComponentInstance::getProjectionMatrix() const
	{
		if (mDirty)
		{
			switch (mProperties.mMode)
			{
				case EOrthoCameraMode::PixelSpace:
				{
					// In this mode we use the rendertarget size to set the left/right/top/bottom planes.
					glm::ivec2 render_target_size = getRenderTargetSize();
					mProjectionMatrix = glm::ortho(0.0f, (float)render_target_size.x, 0.0f, (float)render_target_size.y, mProperties.mNearClippingPlane, mProperties.mFarClippingPlane);
					break;
				}
				case EOrthoCameraMode::CorrectAspectRatio:
				{
					// In this mode, we scale the top and bottom planes based on the aspect ratio
					glm::ivec2 renderTargetSize = getRenderTargetSize();
					float aspect_ratio = (float)renderTargetSize.y / (float)renderTargetSize.x;
					float top_plane = mProperties.mTopPlane * aspect_ratio;
					float bottom_plane = mProperties.mBottomPlane * aspect_ratio;
					mProjectionMatrix = glm::ortho(mProperties.mLeftPlane, mProperties.mRightPlane, bottom_plane, top_plane, mProperties.mNearClippingPlane, mProperties.mFarClippingPlane);
					break;
				}
				case EOrthoCameraMode::Custom:
				{
					mProjectionMatrix = glm::ortho(mProperties.mLeftPlane, mProperties.mRightPlane, mProperties.mBottomPlane, mProperties.mTopPlane, mProperties.mNearClippingPlane, mProperties.mFarClippingPlane);
					break;
				}
			}

			mDirty = false;
		}

		return mProjectionMatrix;
	}


	const glm::mat4 OrthoCameraComponentInstance::getViewMatrix() const
	{
		return glm::inverse(mTransformComponent->getGlobalTransform());
	}


	void OrthoCameraComponent::getDependentComponents(std::vector<rtti::TypeInfo>& components) const
	{
		components.push_back(RTTI_OF(TransformComponent));
	}

}