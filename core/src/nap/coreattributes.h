#pragma once

// Local Includes
#include "rttinap.h"
#include "configure.h"

// External Includes
#include <unordered_map>
#include <vector>

/**
 * coreattributes
 *
 * defines a set of attributes based on basic object types such as float, int etc.
 */
namespace nap
{
    // Typedefs
    using FloatArray = std::vector<float>;
    using StringArray = std::vector<std::string>;
    using IntArray = std::vector<int>;
    using FloatMap = std::unordered_map<std::string, float>;
    using IntMap = std::unordered_map<std::string, int>;
    using StringMap = std::unordered_map<std::string, std::string>;
    using RTTIStringMap = std::unordered_map<rtti::TypeInfo, std::string>;
    using Binary = std::vector<char>;

    // Comparison operator for map and list type attributes
    bool operator==(const FloatArray& a, const FloatArray& b);
    bool operator==(const StringArray& a, const StringArray& b);
    bool operator==(const IntArray& a, const IntArray& b);
    bool operator==(const FloatMap& a, const FloatMap& b);
    bool operator==(const IntMap& a, const IntMap& b);
    bool operator==(const StringMap& a, const StringMap& b);
    bool operator==(const RTTIStringMap& a, const RTTIStringMap& b);
    bool operator==(const Binary& a, const Binary& b);
}
