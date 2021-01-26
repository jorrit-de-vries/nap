#pragma once

// Local Includes
#include "calendaritem.h"

// External Includes
#include <nap/resource.h>
#include <nap/resourceptr.h>

namespace nap
{
	class CalendarInstance;

	/**
	 * Simple calendar, manages a set of calendar items.
	 */
	class NAPAPI Calendar : public Resource
	{
		RTTI_ENABLE(Resource)
	public:
		enum struct EUsage : int
		{
			Static,					///< Calendar can't be updated after initialization, only contains 'Items'.
			Dynamic					///< Calendar can be loaded, updated and saved after initialization.
		};

		virtual ~Calendar();

		/**
		 * Creates and initializes the calendar instance
		 * @return if initialization succeeded
		 */
		virtual bool init(utility::ErrorState& errorState) override;

		/**
		 * @return the calendar instance, only available after initialization
		 */
		CalendarInstance& getInstance()							{ assert(mInstance != nullptr);  return *mInstance; }

		/**
		 * @return the calendar instance, only available after initialization
		 */
		const CalendarInstance& getInstance() const				{ assert(mInstance != nullptr);  return *mInstance; }

		std::vector<nap::ResourcePtr<CalendarItem>> mItems;		///< Property: 'Items' all static calendar items
		Calendar::EUsage mUsage = EUsage::Static;				///< Property: 'Usage' how the calendar is used 

	private:
		std::unique_ptr<CalendarInstance> mInstance = nullptr;	///< Calendar runtime instance
	};


	//////////////////////////////////////////////////////////////////////////
	// Calendar runtime instance
	//////////////////////////////////////////////////////////////////////////

	using CalendarItemList = std::vector<std::unique_ptr<CalendarItem>>;

	/**
	 * Actual runtime version of a simple calendar, created by the resource on initialization.
	 * Allows for inspection, creation, loading and saving of calendar items.
	 * TODO: Use SQLite database for faster item inspection and retrieval.
	 */
	class CalendarInstance final
	{
		friend class Calendar;
		RTTI_ENABLE()
	public:
		// Default constructor
		CalendarInstance();

		// Calendar can't be copied
		CalendarInstance(const CalendarInstance& rhs) = delete;
		CalendarInstance& operator=(const CalendarInstance& rhs) = delete;

		// Calendar can't be moved
		CalendarInstance(const CalendarInstance&& rhs) = delete;
		CalendarInstance& operator=(const CalendarInstance&& rhs) = delete;

	protected:
		/**
		 * Initialize instance against resource
		 * @param calendar the resource to initialize against
		 * @param error contains the error if initialization fails
		 * @return if initialization succeeded
		 */
		bool init(const Calendar& resource, utility::ErrorState& error);

	private:
		CalendarItemList mItems;		///< All the items
	};
}
