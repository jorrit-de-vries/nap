#pragma once

// Local Includes
#include "layoutcomponent.h"

// External Includes
#include <nap/coreattributes.h>

namespace nap
{

	class GridLayout : public LayoutComponent
	{
		RTTI_ENABLE_DERIVED_FROM(LayoutComponent)
	public:
		GridLayout() : LayoutComponent() {}

		bool layout() override;

		Attribute<int> rowCount = {this, "RowCount", 2};
		Attribute<int> colCount = {this, "ColCount", 2};
	};
}

RTTI_DECLARE(nap::GridLayout)
