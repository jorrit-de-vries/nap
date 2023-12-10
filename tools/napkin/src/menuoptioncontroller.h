/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

// External Includes
#include <QMenu>
#include <rtti/typeinfo.h>

namespace napkin
{
	// Forward declares
	template<typename T>
	class MenuOptionController;

	/**
	 * Individual callable menu option for item of base type T
	 */
	template<typename T>
	class MenuOption final
	{
		template<typename T> friend class MenuOptionController;
	public:
		// Menu option callback
		using Callback = std::function<void(T&, QMenu&)>;

		/**
		 * Construct option with possible action
		 * @action action callback
		 */
		MenuOption(Callback&& action) : mCallback(std::move(action)) { }

	private:
		Callback mCallback;
	};


	/**
	 * Collects and assigns menu options for items of type T - grouped by optional type D.
	 */
	template<typename T>
	class MenuOptionController final
	{
	public:
		MenuOptionController() = default;

		/**
		 * Assigns the given callback associated with 'itemType' to a new menu option.
		 * Note that itemType must be derived from base type T.
		 * @param itemType item type associated with given callback
		 * @param action the callback to assign to the menu option
		 */
		void addOption(const nap::rtti::TypeInfo& itemType, typename MenuOption<T>::Callback&& action);

		/**
		 * Assigns the given callback to a new menu option
		 * @param action the callback to assign to the menu option
		 */
		void addOption(typename MenuOption<T>::Callback&& action)					{ addOption(RTTI_OF(T), std::move(action)); }

		/**
		 * Assigns the given callback associated with item D to a new menu option.
		 * Note that D must be derived from base type T.
		 * @param action the callback to assign to the menu option
		 */
		template<typename D>
		void addOption(typename MenuOption<T>::Callback&& action)					{ addOption(RTTI_OF(D), std::move(action)); }

		/**
		 * Populates a menu with options for the given item
		 * @param item the menu item
		 * @param menu the menu to populate
		 */
		void populate(T& item, QMenu& menu);

	private:
		/**
		 * Binds an item of type D (itemType) to a set of possible menu actions
		 */
		struct Binding
		{
			Binding(const nap::rtti::TypeInfo& itemType) : mItemType(itemType) { }
			nap::rtti::TypeInfo mItemType;			///< Node type
			std::vector<MenuOption<T>> mOptions;	///< All available options
		};

		std::vector<Binding> mBindings;	///< All node to callable option bindings
	};


	//////////////////////////////////////////////////////////////////////////
	// Template Definitions
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	void napkin::MenuOptionController<T>::addOption(const nap::rtti::TypeInfo& itemType, typename MenuOption<T>::Callback&& action)
	{
		// Find binding
		auto raw_type = itemType.get_raw_type();
		assert(raw_type.get_raw_type().is_derived_from(RTTI_OF(T)));
		auto& found_binding = std::find_if(mBindings.begin(), mBindings.end(), [&raw_type](const auto& binding) {
				return binding.mItemType == raw_type;
			}
		);

		// Non existing binding
		auto* current_binding = found_binding == mBindings.end() ? nullptr : &(*found_binding);
		if (current_binding == nullptr)
			current_binding = &(mBindings.emplace_back(Binding(itemType.get_raw_type())));

		// Add action
		current_binding->mOptions.emplace_back(std::move(action));
	}


	template<typename T>
	void MenuOptionController<T>::populate(T& item, QMenu& menu)
	{
		// Iterate over all the possible options -> only include options
		// when item is derived from binding type
		for (const auto& binding : mBindings)
		{
			if (item.get_type().get_raw_type().is_derived_from(binding.mItemType))
			{
				for (const auto& option : binding.mOptions)
					option.mCallback(item, menu);
			}
		}
	}
}
