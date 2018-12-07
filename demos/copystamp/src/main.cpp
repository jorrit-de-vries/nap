// main.cpp : Defines the entry point for the console application.
//
// Local Includes
#include "copystampapp.h"

// Nap includes
#include <nap/logger.h>
#include <apprunner.h>

/**
 * Copy Stamp Demo
 * refer to copystampapp.h for a more detailed description of the application
 */
int main(int argc, char *argv[])
{
	// Create core
	nap::Core core;

	// Create app runner using default event handler
	nap::AppRunner<nap::CopystampApp, nap::GUIAppEventHandler> app_runner(core);

	// Start
	nap::utility::ErrorState error;
	if (!app_runner.start(error))
	{
		nap::Logger::fatal("error: %s", error.toString().c_str());
		return -1;
	}

	// Return if the app ran successfully
	return app_runner.exitCode();
}