#pragma once

#include <string>
// Exernal Includes
#include <memory>
#include <vector>

// Local Includes
#include "dllexport.h"

namespace nap
{
	/**
	* Monitors file changes in a directory (currently hardcoded to active directory, and hardcoded to respond to file 'writes').
	* This class works polling-based, so continue to call update to retrieve what files are modified on disk.
	*/
	class NAPAPI DirectoryWatcher
	{
	public:
		DirectoryWatcher();
		~DirectoryWatcher();

		/**
		* Checks if any changes to files were made, returns true if so. Continue to call this function to retrieve 
		* multiple updates.
		* @param modifiedFiles: if the function returns true, contains the filenames of the files that were modified. 
		*/
		bool update(std::vector<std::string>& modifiedFiles);

	private:
        struct PImpl;
        struct PImpl_deleter { void operator()(PImpl*) const; };
        std::unique_ptr<PImpl, PImpl_deleter> mPImpl = nullptr;
	};

} //< End Namespace nap
