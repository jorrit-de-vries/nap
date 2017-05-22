// attribute.h template definitions

namespace nap {
    
    template <typename T>
    const T& AttributeBase::getValue() const
    {
        assert(get_type().is_derived_from<Attribute<T>>());
        auto thisAttribute = static_cast<const Attribute<T>*>(this);
        return thisAttribute->getValue();
    }
    
    
	template <typename T>
	const rtti::TypeInfo Attribute<T>::getValueType() const
	{
		return rtti::TypeInfo::get<T>();
	}


	template <typename T>
	void Attribute<T>::getValue(AttributeBase& inAttribute) const
	{
		static_cast<Attribute<T>&>(inAttribute).setValue(getValue());
	}

    
	template <typename T>
	const T& Attribute<T>::getValue() const
	{
        std::lock_guard<std::mutex> lock(mMutex);
        return mValue;
	}

    
	template <typename T>
	T& Attribute<T>::getValueRef()
	{
		return mValue;
	}

    
	template <typename T>
	void Attribute<T>::setValue(const T& inValue)
	{
        {
            std::lock_guard<std::mutex> lock(mMutex);
            
            // Don't change if it's the same
            if (inValue == mValue)
                return;

            mValue = inValue;
        }

		valueChanged(*this);
	}


	// Sets mValue to @inAttribute.mValue
	template <typename T>
	void Attribute<T>::setValue(const AttributeBase &inAttribute)
	{
        assert(inAttribute.get_type() == get_type());
        
		const Attribute<T>& in_attr = static_cast<const Attribute<T>&>(inAttribute);
        
        {
            std::lock_guard<std::mutex> lock(mMutex);
            std::lock_guard<std::mutex> inAttrLock(in_attr.mMutex);
            
            if (in_attr.mValue == mValue)
                return;

            mValue = in_attr.mValue;
        }

		valueChanged(*this);
	}

    
	//////////////////////////////////////////////////////////////////////////
	// Numeric Attribute Template Definitions
	//////////////////////////////////////////////////////////////////////////

    
	template <typename T>
	void NumericAttribute<T>::setValue(const T& value)
	{
		T new_v = mClamped ? clampValue(value, mMinValue, mMaxValue) : value;
		Attribute<T>::setValue(new_v);
	}


	template <typename T>
	void NumericAttribute<T>::setRange(const T& min, const T& max)
	{
		mMinValue = min;
		mMaxValue = max;
		rangeChanged(*this);
	}


	template <typename T>
	void NumericAttribute<T>::setClamped(bool value)
	{
		// Skip if the same
		if (value == mClamped)
			return;

		// If we're clamping, make sure to update the value
		mClamped = value;
		if (mClamped)
			setValue(Attribute<T>::mValue);
	}

    
	template <typename T>
	void NumericAttribute<T>::getRange(T& outMin, T& outMax) const
	{
		outMin = mMinValue;
		outMax = mMaxValue;
	}

    
	template <typename T>
	NumericAttribute<T>::NumericAttribute(AttributeObject* parent, const std::string& name, const T& value, const T& minValue, const T& maxValue, bool clamped)
		: Attribute<T>(parent, name, value)
	{
		setRange(minValue, maxValue);
		setClamped(clamped);
	}


	// Constructor with default value and no min / max
	template <typename T>
	NumericAttribute<T>::NumericAttribute(AttributeObject* parent, const std::string& name, const T& value)
		: Attribute<T>(parent, name, value)
	{
		setRange(value, value);
	}

    
	//////////////////////////////////////////////////////////////////////////
	// Object attribute link definitions
	//////////////////////////////////////////////////////////////////////////
    
    
	template <typename T>
	T* ObjectLinkAttribute::getTarget()
	{
		Object* target = getTarget();
		if (target == nullptr)
			return nullptr;
		return static_cast<T*>(target);
	}
}
