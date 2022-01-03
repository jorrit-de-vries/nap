/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "computeflockingapp.h"

 // External Includes
#include <nap/core.h>
#include <nap/logger.h>
#include <perspcameracomponent.h>
#include <scene.h>
#include <imgui/imgui.h>
#include <flockingsystemcomponent.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::ComputeFlockingApp)
RTTI_CONSTRUCTOR(nap::Core&)
RTTI_END_CLASS

namespace nap
{
	/**
	* Initialize all the resources and store the objects we need later on
	*/
	bool ComputeFlockingApp::init(utility::ErrorState& error)
	{
		// Create render service
		mRenderService = getCore().getService<RenderService>();
		mInputService = getCore().getService<InputService>();
		mSceneService = getCore().getService<SceneService>();
		mGuiService = getCore().getService<IMGuiService>();

		// Get resource manager and load
		mResourceManager = getCore().getResourceManager();
		if (!mResourceManager->loadFile(getCore().getProjectInfo()->getDataFile(), error))
		{
			Logger::fatal("Unable to deserialize resources: \n %s", error.toString().c_str());
			return false;
		}

		ObjectPtr<Scene> scene = mResourceManager->findObject<Scene>("Scene");
		mRenderWindow = mResourceManager->findObject<RenderWindow>("Window0");
		mCameraEntity = scene->findEntity("CameraEntity");
		mDefaultInputRouter = scene->findEntity("DefaultInputRouterEntity");
		mFlockingSystemEntity = scene->findEntity("FlockingSystemEntity");

		if (!error.check(mFlockingSystemEntity != nullptr, "Missing FlockingSystemEntity"))
			return false;

		mNumBoids = mFlockingSystemEntity->getComponent<FlockingSystemComponentInstance>().mNumBoids;

		mParameterGUI = std::make_unique<ParameterGUI>(getCore());
		mParameterGUI->mParameterGroup = mResourceManager->findObject<ParameterGroup>("FlockingParameters");

		if (!error.check(mParameterGUI->mParameterGroup != nullptr, "Missing ParameterGroup 'FlockingParameters'"))
			return false;

		// Reload the selected preset after hot-reloading 
		mResourceManager->mPreResourcesLoadedSignal.connect(mCacheSelectedPresetSlot);
		mResourceManager->mPostResourcesLoadedSignal.connect(mReloadSelectedPresetSlot);

		// Load the first preset automatically
		auto* parameter_service = getCore().getService<ParameterService>();
		auto presets = parameter_service->getPresets(*mParameterGUI->mParameterGroup);
		if (!parameter_service->getPresets(*mParameterGUI->mParameterGroup).empty())
		{
			if (!mParameterGUI->load(presets[0], error))
				return false;
		}

		mGuiService->selectWindow(mRenderWindow);

		return true;
	}


	void ComputeFlockingApp::reloadSelectedPreset()
	{
		// Load the first preset automatically
		auto* parameter_service = getCore().getService<ParameterService>();
		utility::ErrorState error_state;
		mParameterGUI->load(mSelectedPreset, error_state);
	}


	void ComputeFlockingApp::cacheSelectedPreset()
	{
		auto* parameter_service = getCore().getService<ParameterService>();
		mSelectedPreset = parameter_service->getPresets(*mParameterGUI->mParameterGroup)[mParameterGUI->getSelectedPresetIndex()];
	}


	/**
	 * Forward all received input events to the input router.
	 * The input router is used to filter the input events and to forward them
	 * to the input components of a set of entities, in this case our first person camera.
	 *
	 * We also set up our gui that is drawn at a later stage.
	 */
	void ComputeFlockingApp::update(double deltaTime)
	{
		// Update input
		DefaultInputRouter& input_router = mDefaultInputRouter->getComponent<DefaultInputRouterComponentInstance>().mInputRouter;
		{
			// Update input for first window
			std::vector<nap::EntityInstance*> entities;
			entities.push_back(mCameraEntity.get());

			Window* window = mRenderWindow.get();
			mInputService->processWindowEvents(*window, input_router, entities);
		}

		// Update GUI
		ImGui::Begin("Controls");
		ImGui::Text(getCurrentDateTime().toString().c_str());
		RGBAColorFloat clr = mTextHighlightColor.convert<RGBAColorFloat>();
		ImGui::TextColored(clr, "wasd keys to move, mouse + left mouse button to look");
		ImGui::Text(utility::stringFormat("Framerate: %.02f", getCore().getFramerate()).c_str());
		ImGui::Text(utility::stringFormat("Boids: %d", mNumBoids).c_str());
		mParameterGUI->show(false);
		ImGui::End();
	}


	/**
	 * Render all objects to screen at once
	 * In this case that's only the particle mesh
	 */
	void ComputeFlockingApp::render()
	{
		// Signal the beginning of a new frame, allowing it to be recorded.
		// The system might wait until all commands that were previously associated with the new frame have been processed on the GPU.
		// Multiple frames are in flight at the same time, but if the graphics load is heavy the system might wait here to ensure resources are available.
		mRenderService->beginFrame();

		// Begin recording compute commands
		if (mRenderService->beginComputeRecording())
		{
			mFlockingSystemEntity->getComponent<FlockingSystemComponentInstance>().compute();
			mRenderService->endComputeRecording();
		}

		// Begin recording the render commands for the main render window
		// This prepares a command buffer and starts a render pass
		if (mRenderService->beginRecording(*mRenderWindow))
		{
			// Begin render pass
			mRenderWindow->beginRendering();

			// Render all available geometry
			mRenderService->renderObjects(*mRenderWindow, mCameraEntity->getComponent<PerspCameraComponentInstance>());

			// Render GUI elements
			mGuiService->draw();

			// Stop render pass
			mRenderWindow->endRendering();

			// End recording
			mRenderService->endRecording();
		}

		// Proceed to next frame
		mRenderService->endFrame();
	}


	/**
	* Occurs when the event handler receives a window message.
	* You generally give it to the render service which in turn forwards it to the right internal window.
	* On the next update the render service automatically processes all window events.
	* If you want to listen to specific events associated with a window it's best to listen to a window's mWindowEvent signal
	*/
	void ComputeFlockingApp::windowMessageReceived(WindowEventPtr windowEvent)
	{
		mRenderService->addEvent(std::move(windowEvent));
	}


	/**
	* Called by the app loop. It's best to forward messages to the input service for further processing later on
	* In this case we also check if we need to toggle full-screen or exit the running app
	*/
	void ComputeFlockingApp::inputMessageReceived(InputEventPtr inputEvent)
	{
		if (inputEvent->get_type().is_derived_from(RTTI_OF(nap::KeyPressEvent)))
		{
			// Escape the loop when esc is pressed
			nap::KeyPressEvent* press_event = static_cast<nap::KeyPressEvent*>(inputEvent.get());
			if (press_event->mKey == nap::EKeyCode::KEY_ESCAPE)
			{
				quit();
			}
			// Toggle fullscreen on 'f'
			if (press_event->mKey == nap::EKeyCode::KEY_f)
			{
				mRenderWindow->toggleFullscreen();
			}

		}
		mInputService->addEvent(std::move(inputEvent));
	}
}
