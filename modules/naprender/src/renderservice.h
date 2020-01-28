#pragma once

// External Includes
#include <nap/service.h>
#include <windowevent.h>
#include <rendertarget.h>
#include "vk_mem_alloc.h"

// Local Includes
#include "renderer.h"

namespace opengl
{
	class RenderTarget;
}

namespace nap
{
	// Forward Declares
	class TransformComponentInstance;
	class CameraComponentInstance;
	class RenderableComponentInstance;
	class RenderWindow;
	class RenderService;
	class SceneService;
	class DescriptorSetCache;
	class DescriptorSetAllocator;
	class RenderableMesh;
	class IMesh;
	class MaterialInstance;

	class NAPAPI RenderServiceConfiguration : public ServiceConfiguration
	{
		RTTI_ENABLE(ServiceConfiguration)

	public:
		virtual rtti::TypeInfo getServiceType() override { return RTTI_OF(RenderService); }

		RendererSettings mSettings;		///< Property: 'Settings' All render settings
	};

	/**
	 * Main interface for 2D and 3D rendering operations.
	 * This service initializes the render back-end, manages all vertex array buffers, can be used to create RenderableMesh objects and
	 * provides an interface to render objects to a specific target (screen or back-buffer).
	 * Vertex array object management is handled completely by this service. As a user you only work
	 * with the render interface to render a set of render-able components to a target using a camera.
	 * The service is shut down automatically on exit, and destroys all windows and left over resources.
	 * When rendering geometry using (most) renderObjects() the service automatically sorts your selection based on the blend mode of the material.
	 * Opaque objects are rendered front to back, alpha blended objects are rendered back to front.
	 */
	class NAPAPI RenderService : public Service
	{
		RTTI_ENABLE(Service)

	public:
		using SortFunction = std::function<void(std::vector<RenderableComponentInstance*>&, const CameraComponentInstance&)>;

		/**
		 * Holds current render state 
		 */
		enum class State : int
		{
			Uninitialized	= -1,		///< The render back end is not initialized
			Initialized		= 0,		///< The render back end initialized correctly
			WindowError		= 1,		///< The render back end produced a window error
			SystemError		= 2,		///< The render back end produced a system error
		};

		// Default constructor
		RenderService(ServiceConfiguration* configuration);

		// Default destructor
		virtual ~RenderService();

		/**
		 * Renders all available RenderableComponents in the scene to a specific renderTarget.
		 * The objects to render are sorted using the default sort function (front-to-back for opaque objects, back-to-front for transparent objects).
		 * The sort function is provided by the render service itself, using the default NAP DepthSorter.
		 * Components that can't be rendered with the given camera are omitted. 
		 * @param renderTarget the target to render to
		 * @param camera the camera used for rendering all the available components
		 */
		void renderObjects(opengl::RenderTarget& renderTarget, VkCommandBuffer commandBuffer, CameraComponentInstance& camera);

		/**
		* Renders all available RenderableComponents in the scene to a specific renderTarget.
		* Components that can't be rendered with the given camera are omitted.
		* @param renderTarget the target to render to
		* @param camera the camera used for rendering all the available components
		* @param sortFunction The function used to sort the components to render
		*/
		void renderObjects(opengl::RenderTarget& renderTarget, VkCommandBuffer commandBuffer, CameraComponentInstance& camera, const SortFunction& sortFunction);

		/**
		 * Renders a specific set of objects to a specific renderTarget.
		 * The objects to render are sorted using the default sort function (front-to-back for opaque objects, back-to-front for transparent objects)
		 * The sort function is provided by the render service itself, using the default NAP DepthSorter.
		 * @param renderTarget the target to render to
		 * @param camera the camera used for rendering all the available components
		 * @param comps the components to render to renderTarget
		 */
		void renderObjects(opengl::RenderTarget& renderTarget, VkCommandBuffer commandBuffer, CameraComponentInstance& camera, const std::vector<RenderableComponentInstance*>& comps);

		/**
		* Renders a specific set of objects to a specific renderTarget.
		*
		* @param renderTarget the target to render to
		* @param camera the camera used for rendering all the available components
		* @param comps the components to render to renderTarget
		* @param sortFunction The function used to sort the components to render
		*/
		void renderObjects(opengl::RenderTarget& renderTarget, VkCommandBuffer commandBuffer, CameraComponentInstance& camera, const std::vector<RenderableComponentInstance*>& comps, const SortFunction& sortFunction);

		/**
		 * Shuts down the managed renderer
		 */
		virtual void shutdown() override;

		/**
		 * Add a new window for the specified resource
		 * @param window the window to add as a valid render target
		 * @param errorState contains the error message if the window could not be added
		 */
		std::shared_ptr<GLWindow> addWindow(RenderWindow& window, utility::ErrorState& errorState);

		/**
		 * Remove a window
		 * @param window the window to remove from the render service
		 */
		void removeWindow(RenderWindow& window);

		/**
		 * Find a RenderWindowResource by its native handle
		 * @param nativeWindow the native window handle (i.e. the SDL_Window pointer)
		 * @return the render window associated with the native window
		 */
		RenderWindow* findWindow(void* nativeWindow) const;

		/**
		 * Find a RenderWindow based on a window number.
		 * @param id the number of the window to find.
		 * @return the window, nullptr if not found
		 */
		RenderWindow* findWindow(uint id) const;

		/**
		 * Add a window event that is processed later, ownership is transferred here.
		 * The window number in the event is used to find the right render window to forward the event to.
		 * @param windowEvent the window event to add.
		 */
		void addEvent(WindowEventPtr windowEvent);

		/**
		* Creates a renderable mesh that represents the coupling between a mesh and material that can be rendered to screen.
		* Internally the renderable mesh manages a vertex array object that is issued by the render service.
		* This function should be called from on initialization of components that work with meshes and materials: ie: all types of RenderableComponent. 
		* The result should be validated by calling RenderableMesh.isValid(). Invalid mesh / material representations can't be rendered together.
		* @param mesh The mesh that is used in the mesh-material combination.
		* @param materialInstance The material instance that is used in the mesh-material combination.
		* @param errorState If this function returns an invalid renderable mesh, the error state contains error information.
		* @return A RenderableMesh object that can be used in setMesh calls. Check isValid on the object to see if creation succeeded or failed.
		*/
		RenderableMesh createRenderableMesh(IMesh& mesh, MaterialInstance& materialInstance, utility::ErrorState& errorState);

		void recreatePipeline(RenderableMesh& renderableMesh, VkPipelineLayout& layout, VkPipeline& pipeline);

		void advanceToFrame(int frameIndex);

		Renderer& getRenderer() { return *mRenderer; }

		DescriptorSetCache& getOrCreateDescriptorSetCache(VkDescriptorSetLayout layout);

		VmaAllocator getVulkanAllocator() { return mVulkanAllocator; }

		int getCurrentFrameIndex() const { return mCurrentFrameIndex; }

		VkRenderPass getOrCreateRenderPass(ERenderTargetFormat format);
		
	protected:
		/**
		* Object creation registration
		*/
		virtual void registerObjectCreators(rtti::Factory& factory) override;

		/**
		 * Register dependencies, render module depends on scene
		 */
		virtual void getDependentServices(std::vector<rtti::TypeInfo>& dependencies) override;

		/**
		 * Initialize the renderer, the service owns the renderer.
		 * @param errorState contains the error message if the service could not be initialized
		 * @return if the service has been initialized successfully
		 */
		virtual bool init(nap::utility::ErrorState& errorState) override;

		/**
		 * Called before update, ensures the primary window is the active window before update is called.
		 * @param deltaTime time in seconds in between frames.
		 */
		virtual void preUpdate(double deltaTime) override;

		/**
		 * Process all received window events.
		 * @param deltaTime time in seconds in between frames.
		 */
		virtual void update(double deltaTime) override;

    private:
		/**
		* Sorts a set of renderable components based on distance to the camera, ie: depth
		* Note that when the object is of a type mesh it will use the material to sort based on opacity
		* If the renderable object is not a mesh the sorting will occur front-to-back regardless of it's type as we don't
		* know the way the object is rendered to screen
		* @param comps the renderable components to sort
		* @param camera the camera used for sorting based on distance
		*/
		void sortObjects(std::vector<RenderableComponentInstance*>& comps, const CameraComponentInstance& camera);

		/**
		 * Processes all window related events for all available windows
		 */
		void processEvents();

		struct PipelineToDestroy
		{
			int			mFrameIndex;
			VkPipeline	mPipeline;
		};

		using WindowList = std::vector<RenderWindow*>;
		using PipelineList = std::vector<PipelineToDestroy>;
		using DescriptorSetCacheMap = std::unordered_map<VkDescriptorSetLayout, std::unique_ptr<DescriptorSetCache>>;

		std::unique_ptr<nap::Renderer>			mRenderer;												//< Holds the currently active renderer
		VmaAllocator							mVulkanAllocator;
		WindowList								mWindows;												//< All available windows
		SceneService*							mSceneService = nullptr;								//< Service that manages all the scenes

		PipelineList							mPipelinesToDestroy;

		int										mCurrentFrameIndex = 0;
		DescriptorSetCacheMap					mDescriptorSetCaches;
		std::unique_ptr<DescriptorSetAllocator> mDescriptorSetAllocator;

		VkRenderPass							mRenderPassRGBA8 = nullptr;
		VkRenderPass							mRenderPassRGB8 = nullptr;
		VkRenderPass							mRenderPassR8 = nullptr;
		VkRenderPass							mRenderPassDepth = nullptr;
	};
} // nap



