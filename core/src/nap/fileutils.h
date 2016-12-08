#pragma once

// External Includes
#include <string>
#include <vector>

namespace nap
{
    /**
     * List all files in a directory
     * @param directory The directory to search in
     * @param outFilenames A vector of files to populate with absolute filenames
     * @return False on faillure
     */
	bool listDir(const char* directory, std::vector<std::string>& outFilenames);

    /**
     * Given a relative path, return an absolute path
     * @param relPath The path to convert
     * @return An absolute file path
     */
	std::string getAbsolutePath(const std::string& relPath);

    /**
     * Return the extension of the given filename. Eg. "my.directory/myFile.tar.gz" -> "gz"
     * @param filename The filename to get the extension from
     * @return The file extension without the dot
     */
	std::string getFileExtension(const std::string& filename);

	/**
	 * @Return the name of the given file with extension, empty string if file has no extension
	 * @param file the file to extract the name frame
	 */
	std::string getFileName(const std::string& file);

	/**
	* @return file name without extension
	* @param file path that is stripped
	*/
	std::string getFileNameWithoutExtension(const std::string& file);

	/**
	 * @param file the file to strip the extension from
	 */
	void stripFileExtension(std::string& file);

	/**
	 * @return file without extension
	 * @param file the file to string the extension from
	 */
	std::string stripFileExtension(const std::string& file);

	/**
	 * @return if the file has the associated file extension
	 * @param file the file to check extension for
	 * @param extension the file extension without preceding '.'
	 */
	bool hasExtension(const std::string& file, const std::string& extension);

    /**
     * Check whether a file exists or not.
     * @param filename The absolute or relative file path to check for
     * @return true if the file exists, false otherwise
     */
    bool fileExists(const std::string& filename);
}