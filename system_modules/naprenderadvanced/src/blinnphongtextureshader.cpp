/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "blinnphongtextureshader.h"
#include "renderadvancedservice.h"

// Local includes
#include <nap/core.h>

// nap::BlinnPhongTextureShader run time class definition 
RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::BlinnPhongTextureShader)
	RTTI_CONSTRUCTOR(nap::Core&)
	RTTI_PROPERTY("QuadSampleCount", &nap::BlinnPhongTextureShader::mQuadSampleCount, nap::rtti::EPropertyMetaData::Default)
	RTTI_PROPERTY("CubeSampleCount", &nap::BlinnPhongTextureShader::mCubeSampleCount, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

//////////////////////////////////////////////////////////////////////////


namespace nap
{
	namespace shader
	{
		inline constexpr const char* blinnphongtexture = "blinnphongtexture";
		inline constexpr const char* blinnphongtexturepi = "blinnphongtexturepi";

		namespace constant
		{
			inline constexpr const char* QUAD_SAMPLE_COUNT = "QUAD_SAMPLE_COUNT";
			inline constexpr const char* CUBE_SAMPLE_COUNT = "CUBE_SAMPLE_COUNT";
		}
	}

	BlinnPhongTextureShader::BlinnPhongTextureShader(Core& core) :
		Shader(core), mRenderAdvancedService(core.getService<RenderAdvancedService>())
	{ }


	bool BlinnPhongTextureShader::init(utility::ErrorState& errorState)
	{
#ifdef RENDERADVANCED_RPI
		const std::string shader_name = shader::blinnphongtexturepi;
#else
		const std::string shader_name = shader::blinnphongtexture;
#endif

		std::string relative_path = utility::joinPath({ "shaders", utility::appendFileExtension(shader_name.c_str(), "vert") });
		const std::string vertex_shader_path = mRenderAdvancedService->getModule().findAsset(relative_path);
		if (!errorState.check(!vertex_shader_path.empty(), "%s: Unable to find %s vertex shader %s", mRenderAdvancedService->getModule().getName().c_str(), shader_name.c_str(), vertex_shader_path.c_str()))
			return false;

		relative_path = utility::joinPath({ "shaders", utility::appendFileExtension(shader::blinnphongtexture, "frag") });
		const std::string fragment_shader_path = mRenderAdvancedService->getModule().findAsset(relative_path);
		if (!errorState.check(!vertex_shader_path.empty(), "%s: Unable to find %s fragment shader %s", mRenderAdvancedService->getModule().getName().c_str(), shader_name.c_str(), fragment_shader_path.c_str()))
			return false;

		// Read vert shader file
		std::string vert_source;
		if (!errorState.check(utility::readFileToString(vertex_shader_path, vert_source, errorState), "Unable to read %s vertex shader file", shader_name.c_str()))
			return false;

		// Read frag shader file
		std::string frag_source;
		if (!errorState.check(utility::readFileToString(fragment_shader_path, frag_source, errorState), "Unable to read %s fragment shader file", shader_name.c_str()))
			return false;

		// Copy data search paths
		const auto search_paths = mRenderAdvancedService->getModule().getInformation().mDataSearchPaths;

		// Compile shader
		if (!load(shader_name, search_paths, vert_source.data(), vert_source.size(), frag_source.data(), frag_source.size(), errorState))
			return false;

		// Set shadow map sample counts
		if (!setFragmentSpecializationConstant(shader::constant::QUAD_SAMPLE_COUNT, std::max<uint>(mQuadSampleCount, 1), errorState))
			return false;

		if (!setFragmentSpecializationConstant(shader::constant::CUBE_SAMPLE_COUNT, std::max<uint>(mCubeSampleCount, 1), errorState))
			return false;

		return true;
	}
}
