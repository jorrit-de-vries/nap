/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External Includes
#include <nap/service.h>
#include <entity.h>
#include <nap/datetime.h>
#include <rtti/factory.h>

namespace nap
{
	class NAPAPI SequenceGuiService : public Service
	{
		friend class SequencePlayerOutput;

		RTTI_ENABLE(Service)
	public:
		/**
		 * Constructor
		 */
		SequenceGuiService(ServiceConfiguration* configuration);

		/**
		 * Deconstructor
		 */
		~SequenceGuiService() override;

		/**
		 * registers object creator method that can be passed on to the rtti factory
		 * @param objectCreator unique pointer to method
		 */
		static bool registerObjectCreator(std::unique_ptr<rtti::IObjectCreator>(*objectCreator)(SequenceGuiService*));
	protected:
		/**
		 * registers all objects that need a specific way of construction
		 * @param factory the factory to register the object creators with
		 */
		void registerObjectCreators(rtti::Factory& factory) override;

		/**
		 * initializes service
		 * @param errorState contains any errors
		 * @return returns true on successful initialization
		 */
		bool init(nap::utility::ErrorState& errorState) override;

		/**
		 * updates any outputs and editors
		 * @param deltaTime deltaTime
		 */
		void update(double deltaTime) override;
	private:
	};
}
