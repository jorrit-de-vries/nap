#include "../directorywatcher.h"

#include <nap/fileutils.h>

#include "assert.h"
#include <CoreServices/CoreServices.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>

#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace nap {
    
    /**
     * Internal data container to hide internals from the header.
     */
    struct DirectoryWatcher::PImpl
    {
        // the files that have been modified and not yet reported to the user of the system
        std::vector<std::string> modifiedFiles;
        
        FSEventStreamContext context; // could put stream-specific data here.
        CFAbsoluteTime latency = 1; // Latency in seconds
        std::string executablePath; // path to the current executable
        
        CFArrayRef pathsToWatch;
        FSEventStreamRef stream;
        CFRunLoopRef runLoop;
        std::unique_ptr<std::thread> watchThread = nullptr;
    };
    
    
    /**
     * Callback called by the event stream to handle file change events
     */
    
    void scanCallback(ConstFSEventStreamRef streamRef,
                      void *clientCallBackInfo,
                      size_t numEvents,
                      void *eventPaths,
                      const FSEventStreamEventFlags eventFlags[],
                      const FSEventStreamEventId eventIds[])
    {
        char **paths = (char**)eventPaths;
        std::vector<std::string>* modifiedFiles = (std::vector<std::string>*)(clientCallBackInfo);
        std::cout << "callback" << std::endl;
        
        for (auto i = 0; i < numEvents; i++) {
            modifiedFiles->emplace_back(std::string(paths[i]));
        }
    }
    
    
    /**
     * Deleter operator definition
     */
    void DirectoryWatcher::PImpl_deleter::operator()(DirectoryWatcher::PImpl*ptr) const { delete ptr; }

    
	/**
	* Installs monitor: opens directory, creates event, starts directory scan.
	*/
	DirectoryWatcher::DirectoryWatcher()
	{
        mPImpl = std::unique_ptr<PImpl, PImpl_deleter>(new PImpl);
        
        mPImpl->watchThread = std::make_unique<std::thread>([&](){
            
            // retain and release have to be set to NULL explicitly otherwise this causes irregular crashes
            mPImpl->context.version = 0;
            mPImpl->context.info = (void*)(&mPImpl->modifiedFiles);
            mPImpl->context.retain = NULL;
            mPImpl->context.release = NULL;
            mPImpl->context.copyDescription = NULL;
            
            // retrieve the path to the current executable
            uint32_t size = 256;
            std::vector<char> buffer;
            buffer.resize(size);
            _NSGetExecutablePath(buffer.data(), &size);
            mPImpl->executablePath = getFileDir(std::string(buffer.data()));
            
            std::string dirToWatch = mPImpl->executablePath;
            CFStringRef pathToWatchCF = CFStringCreateWithCString(NULL, dirToWatch.c_str(), kCFStringEncodingUTF8);
            mPImpl->pathsToWatch = CFArrayCreate(NULL, (const void **)&pathToWatchCF, 1, NULL);
            
            // create event stream for file change event
            mPImpl->stream = FSEventStreamCreate(NULL,
                                                 &scanCallback,
                                                 &mPImpl->context,
                                                 mPImpl->pathsToWatch,
                                                 kFSEventStreamEventIdSinceNow,
                                                 mPImpl->latency,
                                                 kFSEventStreamCreateFlagFileEvents
                                                 );
            
            mPImpl->runLoop = CFRunLoopGetCurrent();
            // schedule the stream on the current run loop
            FSEventStreamScheduleWithRunLoop(mPImpl->stream, mPImpl->runLoop, kCFRunLoopDefaultMode);
            // tells the stream to begin sending events
            FSEventStreamStart(mPImpl->stream);
            
            CFRunLoopRun();
        });
	}


	/**
	* Cleanup
	*/
	DirectoryWatcher::~DirectoryWatcher()
	{
        CFRunLoopStop(mPImpl->runLoop);
        mPImpl->watchThread->join();
        FSEventStreamStop(mPImpl->stream);
        FSEventStreamInvalidate(mPImpl->stream);
        FSEventStreamRelease(mPImpl->stream);
	}


	/**
	* Checks if any changes to files were made, returns true if so. Continue to call this function to retrieve
	* multiple updates.
	*/
	bool DirectoryWatcher::update(std::vector<std::string>& modifiedFiles)
	{
        if (mPImpl->modifiedFiles.empty())
            return false;
        
        for (auto& modifiedFile : mPImpl->modifiedFiles)
        {
            // check if the executable path is found at the start if the modifiel file's path
            auto pos = modifiedFile.find(mPImpl->executablePath + "/");
            if (pos == 0)
            {
                // strip the executable's path from the start
                modifiedFile.erase(0, mPImpl->executablePath.size() + 1);
                modifiedFiles.emplace_back(modifiedFile);
            }
        }
        
        mPImpl->modifiedFiles.clear();
        
        // if the modified files found by the event stream are not found in the executable's dir we still need to return false
        return !modifiedFiles.empty();
	}
}
