/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External Includes
#include <shader.h>

namespace nap
{
	// Forward declares
	class Core;
	class Material;

	// Uniform names
	namespace uniform
	{
		namespace sampler
		{	
			inline constexpr const char* colorTexture	= "colorTexture";		///< Name of the color texture sampler
		}

		inline constexpr const char* uboStruct = "UBO";							///< UBO that contains all the uniforms
		inline constexpr const char* textureSize = "textureSize";				///<
		inline constexpr const char* direction = "direction";					///<
		inline constexpr const char* scaleBias = "scaleBias";					///<
	}

	/**
	 * Shader that renders glyphs. Used by the nap::RenderableTextComponent
	 */
	class NAPAPI BlurShader : public Shader
	{
		RTTI_ENABLE(Shader)
	public:
		BlurShader(Core& core);

		/**
		 * Cross compiles the font GLSL shader code to SPIR-V, creates the shader module and parses all the uniforms and samplers.
		 * @param errorState contains the error if initialization fails.
		 * @return if initialization succeeded.
		 */
		virtual bool init(utility::ErrorState& errorState) override;

	private:
		RenderService* mRenderService = nullptr;
	};
}
