#pragma once

#include "nbackbufferrendertarget.h"
#include <rtti/rtti.h>
#include <nap/numeric.h>

// External Includes
#include <string.h>
#include <glm/glm.hpp>
#include <utility/dllexport.h>
#include "SDL_video.h"
#include "vulkan/vulkan_core.h"

struct SDL_Window;
typedef void *SDL_GLContext;

namespace nap
{
	namespace utility
	{
		class ErrorState;
	}

	// Forward Declares
	class GLWindow;
	class Renderer;

	/**
	* Holds all window launch settings
	* Note that this object is only used when constructing the window
	* Use the size, position and title attributes on the component
	* to position the window
	*/
	struct NAPAPI RenderWindowSettings
	{
	public:
		// Construction / Destruction
		RenderWindowSettings() = default;
		virtual ~RenderWindowSettings() = default;

		std::string		title;										///< Name of the window
		int				x				= SDL_WINDOWPOS_CENTERED;	///< Position
		int				y				= SDL_WINDOWPOS_CENTERED;	///< Position
		int				width			= 512;						//< Width of the window
		int				height			= 512;						///< Height of the window
		bool			borderless		= false;					///< If the window is borderless
		bool			resizable		= true;						///< If the window is resizable
		bool			visible			= true;						///< If the window is visible or not
		bool			sync			= true;						///< If v-sync is turned on for the window
		bool			highdpi			= true;						///< If high-dpi mode is enabled
	};


	/**
	 * An OpenGL Accelerated Render Window. 
	 * This is a low level construct and a member of the RenderWindow resource.
	 * Declare that resource to create an OpenGL accelerated render window.
	 */
	class NAPAPI GLWindow final
	{
		RTTI_ENABLE()
		friend class Renderer;
	public:

		GLWindow();
		~GLWindow();

		/**
		* Delete copy construction
		*/
		GLWindow(const GLWindow& other) = delete;
		GLWindow& operator=(const GLWindow& other) = delete;

		bool init(const RenderWindowSettings& settings, Renderer& renderer, utility::ErrorState& errorState);

		/**
		* @return the hardware window handle, nullptr if undefined
		*/
		SDL_Window* getNativeWindow() const;

		/**
		 * The back buffer for an OpenGL window isn't an actual frame buffer
		 * but allows for handling windows and render targets inside the framework
		 * in a similar way. Associating a back buffer with a window also ensures, in this case,
		 * that the opengl viewport always matches the window dimensions
		 * @return the back buffer associated with this window
		 */
		const opengl::BackbufferRenderTarget& getBackbuffer() const;

		/**
		* The back buffer for an OpenGL window isn't an actual frame buffer
		* but allows for handling windows and render targets inside the framework
		* in a similar way. Associating a back buffer with a window also ensures, in this case,
		* that the opengl viewport always matches the window dimensions
		* @return the back buffer associated with this window
		*/
		opengl::BackbufferRenderTarget& getBackbuffer();

		/**
		 * Set the window title
		 * @param title the new window title
		 */
		void setTitle(const std::string& title);

		/**
		 * Set the window position
		 * @param position the window position coordinates in pixels
		 */
		void setPosition(const glm::ivec2& position);

		/**
		 *	@return the window position in pixel coordinates
		 */
		glm::ivec2 getPosition();

		/**
		 * Set the window size
		 * @param size the new window size in pixels
		 */
		void setSize(const glm::ivec2& size);

		/**
		 * @ the window size in pixels
		 */
		const glm::ivec2 getSize() const;

		/**
		 * Makes the window full screen
		 * @param value if the window should be full screen or not
		 */
		void setFullScreen(bool value);

		/**
		 * Show window
		 */
		void showWindow();

		/**
		 * Hide window
		 */
		void hideWindow();

		/**
		 * Swap the OpenGL buffers for a window, if double-buffering is supported
		 * This call also ensures all the previously made OpenGL calls are pushed
		 * and processed on the GPU before swapping buffers
		 */
		void swap();

		/**
		 * Make this window active
		 */
		void makeCurrent();

		/**
		 *	Returns the window number
		 */
		uint32 getNumber() const;

		VkCommandBuffer	getCommandBuffer();

		int getCurrentFrameIndex() const { return mCurrentFrame; }

	private:
		opengl::BackbufferRenderTarget					mBackbuffer;

		VkDevice										mDevice = nullptr;
		VkSurfaceKHR									mSurface = nullptr;
		VkSwapchainKHR									mSwapchain = nullptr;
		VkRenderPass									mRenderPass = nullptr;
		VkQueue											mGraphicsQueue = nullptr;
		VkQueue											mPresentQueue = nullptr;
		std::vector<VkImageView>						mSwapChainImageViews;
		std::vector<VkFramebuffer>						mSwapChainFramebuffers;
		std::vector<VkCommandBuffer>					mCommandBuffers;
		std::vector<VkSemaphore>						mImageAvailableSemaphores;
		std::vector<VkSemaphore>						mRenderFinishedSemaphores;
		std::vector<VkFence>							mInFlightFences;
		VkImage											mDepthImage = nullptr;
		VkDeviceMemory									mDepthImageMemory = nullptr;
		VkImageView										mDepthImageView = nullptr;
		int												mCurrentFrame = 0;
		uint32_t										mCurrentImageIndex = 0;

		SDL_Window*										mWindow = nullptr;		// Actual GL window

		/**
		 * Apply the specified window settings. Normally this is done during initialization of new windows,
		 * but for real-time editing scenarios we need this call to update the primary window which has been created before
		 * @param settings The settings to apply
		 */
		void applySettings(const RenderWindowSettings& settings);
	};
}
