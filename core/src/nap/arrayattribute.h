#pragma once

//
//  ArrayAttribute.hpp
//  Soundlab
//
//  Created by Stijn van Beek on 10/3/16.
//
//

// External includes
#include <stdio.h>

// Local includes
#include <rtti/rtti.h>
#include "attribute.h"

namespace nap
{

    // Forward Declares
    class AttributeObject;
    
    /**
     ArrayAttributeBase
     
     */
    class ArrayAttributeBase : public AttributeBase
    {
        RTTI_ENABLE_DERIVED_FROM(nap::AttributeBase)
    public:
        // Constructor
        ArrayAttributeBase() = default;
        ArrayAttributeBase(AttributeObject* parent, const std::string& name, bool atomic = false) : AttributeBase(parent, name, atomic) {}
        
        // Enable default copy behavior
        ArrayAttributeBase(ArrayAttributeBase&) = default;
        ArrayAttributeBase& operator=(const ArrayAttributeBase&) = default;
        
        // Virtual destructor because of virtual methods!
        virtual ~ArrayAttributeBase() = default;
        
        virtual int getSize() = 0;
    };
    
    
    /**
     ArrayAttribute
     
     Concrete implementation of array attribute, invisible to UI and scripting
     **/
    template <typename T>
    class ArrayAttribute : public ArrayAttributeBase
    {
        RTTI_ENABLE_DERIVED_FROM(ArrayAttributeBase)
    public:
        // Default constructor
        ArrayAttribute() : ArrayAttributeBase() {  }
        
        // Constructor with defult value
        ArrayAttribute(AttributeObject* parent, const std::string& name, const std::vector<T>& inValue, bool atomic = false)
        : ArrayAttributeBase(parent, name, atomic), mValue(inValue)
        {
            setValueSlot.setFunction({ [this](const std::vector<T>& value) { this->setValue(value); } });
        }
        
        // Constructor without default value
        ArrayAttribute(AttributeObject* parent, const std::string& name) : ArrayAttributeBase(parent, name, false)
        {
            setValueSlot.setFunction({ [this](const std::vector<T>& value) { this->setValue(value); } });
        }
        
        // Constructor to declare an attribute with a member function pointer for the @valueChangedSignal as last argument.
        template <typename U, typename F>
        ArrayAttribute(U* parent, const std::string& name, const std::vector<T>& inValue, F function, bool atomic = false)
        : ArrayAttributeBase(parent, name, atomic), mValue(inValue)
        {
            setValueSlot.setFunction({ [this](const std::vector<T>& value) { this->setValue(value); } });
            valueChanged.connect(parent, function);
        }
        
        virtual const RTTI::TypeInfo getValueType() const override;
        
        // Getters
        virtual void getValue(AttributeBase& inAttribute) const override;
        const std::vector<T>& getValue() const;
        std::vector<T>& getValueRef();
        const T& getValue(int index) const;
        
        // Setters
        virtual void setValue(const AttributeBase &inAttribute) override;
        virtual void setValue(const std::vector<T>& inValue);
        void setValue(int index, const T& inValue);
        
        int getSize() override final { return mValue.size(); }
        
        // Adds a new @element to the end of the array
        void add(const T& element);
        // Insert a new element at @index
        void insert(int index, const T& element);
        // Remove element at @index
        void remove(int index);
        // Clear the array
        void clear();
        
        const typename std::vector<T>::iterator begin() { return mValue.begin(); }
        const typename std::vector<T>::iterator end() { return mValue.end(); }
        
        // This slot will be invoked when the value is set
        Slot<const std::vector<T>&>		setValueSlot;
        
        // Operator overloads
        operator const std::vector<T>&() const { return getValue(); }

        const T& operator [] (int index) const { return mValue[index]; }
        
    protected:
        // Members
        std::vector<T> mValue;
        
    private:
        // Keep constructor hidden, use factory methods to instantiate
        ArrayAttribute(const std::vector<T>& inValue) : mValue(inValue) {}
    };
    
    
    
    // TEMPLATE DEFINITIONS
    
    template <typename T>
    const RTTI::TypeInfo ArrayAttribute<T>::getValueType() const
    {
        return RTTI::TypeInfo::get<T>();
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::getValue(AttributeBase& inAttribute) const
    {
        assert(getTypeInfo().isKindOf(inAttribute.getTypeInfo()));
        
        static_cast<ArrayAttribute<T>&>(inAttribute).setValue(getValue());
    }
    
    
    template <typename T>
    const std::vector<T>& ArrayAttribute<T>::getValue() const
    {
        return mValue;
    }
    
    
    template <typename T>
    std::vector<T>& ArrayAttribute<T>::getValueRef()
    {
        return mValue;
    }
    
    
    template <typename T>
    const T& ArrayAttribute<T>::getValue(int index) const
    {
        return mValue.at(index);
    }

    
    template <typename T>
    void ArrayAttribute<T>::setValue(const std::vector<T>& inValue)
    {
        mValue = inValue;
        valueChanged(*this);
    }
    
    
    // Sets mValue to @inAttribute.mValue
    template <typename T>
    void ArrayAttribute<T>::setValue(const AttributeBase &inAttribute)
    {
        assert(inAttribute.getTypeInfo() == getTypeInfo());
        const ArrayAttribute<T>& in_attr = static_cast<const ArrayAttribute<T>&>(inAttribute);

        mValue = in_attr.mValue;
        
        valueChanged(*this);
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::setValue(int index, const T& inValue)
    {
        mValue[index] = inValue;
        
        valueChanged(*this);
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::add(const T& element)
    {
        mValue.emplace_back(element);
        
        valueChanged(*this); 
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::insert(int index, const T& element)
    {
        auto it = mValue.begin();
        std::advance(it, index);
        mValue.insert(it, element);
        
        valueChanged(*this);
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::remove(int index)
    {
        auto it = mValue.begin();
        std::advance(it, index);
        mValue.erase(it);
        
        valueChanged(*this);
    }
    
    
    template <typename T>
    void ArrayAttribute<T>::clear()
    {
        mValue.clear();
        valueChanged(*this);
    }
    
    
}

RTTI_DECLARE_BASE(nap::ArrayAttributeBase)

