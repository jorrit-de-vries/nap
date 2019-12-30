// Local Includes
#include <nap/core.h>
#include <nap/logger.h>

// External Includes
#include <rtti/jsonreader.h>
#include <utility/fileutils.h>
#include <nap/simpleserializer.h>

namespace nap
{
	static const std::string sPossibleProjectParents[] =
			{
					"projects",     // User projects against packaged NAP
					"examples",     // Example projects
					"demos",        // Demo projects
					"apps",         // Applications in NAP source
					"test"          // Old test projects in NAP source
			};


	bool Core::findProjectFilePath(const std::string& filename, std::string& foundFilePath) const
	{
		const std::string exeDir = utility::getExecutableDir();

		// Check for the file in its normal location, beside the binary
		const std::string alongsideBinaryPath = utility::getExecutableDir() + "/" + filename;
		nap::Logger::debug("Looking for '%s'...", alongsideBinaryPath.c_str());
		if (utility::fileExists(alongsideBinaryPath))
		{
			foundFilePath = alongsideBinaryPath;
			return true;
		}
#ifndef NAP_PACKAGED_BUILD
		// When working against NAP source find our file in the tree structure in the project source.
		// This is effectively a workaround for wanting to keep all binaries in the same root folder on Windows
		// so that we avoid module DLL copying hell.

		const std::string napRoot = utility::getAbsolutePath(exeDir + "/../../");
		const std::string projectName = utility::getFileNameWithoutExtension(utility::getExecutablePath());

		// Iterate possible project locations
		for (auto& parentPath : sPossibleProjectParents)
		{
			std::string testDataPath = napRoot + "/" + parentPath + "/" + projectName;
			nap::Logger::debug("Looking for project.json in '%s'...", testDataPath.c_str());
			if (!utility::dirExists(testDataPath))
				continue;

			// We found our project folder, now let's verify we have a our file in there
			testDataPath += "/";
			testDataPath += filename;
			if (utility::fileExists(testDataPath))
			{
				foundFilePath = testDataPath;
				nap::Logger::debug("Found '%s'...", foundFilePath.c_str());
				return true;
			}
		}
#endif // NAP_PACKAGED_BUILD
		return false;
	}


	bool Core::hasServiceConfiguration()
	{
		std::string config_file_path;
		return findProjectFilePath(SERVICE_CONFIG_FILENAME, config_file_path);
	}


	bool Core::loadServiceConfiguration(rtti::DeserializeResult& deserializeResult, utility::ErrorState& errorState)
	{
		std::string config_file_path;
		if (!findProjectFilePath(SERVICE_CONFIG_FILENAME, config_file_path))
			return false;

		return rtti::readJSONFile(config_file_path, rtti::EPropertyValidationMode::DisallowMissingProperties,
								  rtti::EPointerPropertyMode::NoRawPointers, mResourceManager->getFactory(),
								  deserializeResult, errorState);
	}

	bool nap::Core::loadServiceConfiguration(const nap::ProjectInfo& projectInfo,
											 ServiceConfigMap& serviceConfigs,
											 nap::utility::ErrorState& errorState)
	{
		const char* serviceConfigKey = "service_configurations";

		rapidjson::Document document;
		if (!loadJSONDocument(projectInfo.getFilename(), document, errorState))
			return false;

		// If the project file doesn't have a serviceConfigurations section, bail out and report success
		if (!document.HasMember(serviceConfigKey))
			return true;

		// Read objects
		rtti::DeserializeResult deserialize_result;
		if (!rtti::deserializeObjects(document[serviceConfigKey],
									  rtti::EPropertyValidationMode::DisallowMissingProperties,
									  rtti::EPointerPropertyMode::NoRawPointers,
									  mResourceManager->getFactory(),
									  deserialize_result, errorState))
			return false;

		// Move the loaded service configurations into the given ServiceConfigMap
		for (auto& object : deserialize_result.mReadObjects)
		{
			if (!errorState.check(object->get_type().is_derived_from<ServiceConfiguration>(),
								  "Expected only contain ServiceConfigurations"))
				return false;

			std::unique_ptr<ServiceConfiguration> config = rtti_cast<ServiceConfiguration>(object);
			serviceConfigs[config->getServiceType()] = std::move(config);
		}

		return true;
	}

}