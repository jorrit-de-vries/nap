#pragma once

// Midi includes
#include "midievent.h"
#include "midiservice.h"

// Nap includes
#include <nap/component.h>
#include <nap/signalslot.h>


namespace nap
{
    
    class MidiInputComponentInstance;
    

    /**
     * Component that filters incoming midi messages and makes them available for other components by emitting a signal.
     */
    class NAPAPI MidiInputComponent : public nap::Component
    {
        RTTI_ENABLE(nap::Component)
        DECLARE_COMPONENT(MidiInputComponent, MidiInputComponentInstance)
        
    public:
        MidiInputComponent() : nap::Component() { }
        
        std::vector<MidiValue> mPorts; /**< Filter specifying input ports that will be listened to. Empty means all ports. */
        std::vector<MidiValue> mChannels; /**< Filter specifying what midi channels to listen to. Empty means all channels. */
        std::vector<MidiValue> mNumbers; /**< Filter specifying what number bytes (like cc numbers) to listen to. Empty means all numbers. */
        std::vector<MidiEvent::Type> mTypes; /**< Filter specifying what event types to listen to. Empty means all types. */
    };

    
    /**
     * Instance of component that filters incoming midi messages and makes them available for other components by emitting a signal.
     */
    class NAPAPI MidiInputComponentInstance : public nap::ComponentInstance
    {
        RTTI_ENABLE(nap::ComponentInstance)
        friend class MidiService;
        
    public:
        MidiInputComponentInstance(EntityInstance& entity, Component& resource) : nap::ComponentInstance(entity, resource) { }
        
        // Initialize the component
        bool init(utility::ErrorState& errorState) override;
        
        virtual ~MidiInputComponentInstance();
        
        /**
         * Signal emitted when a midi message is received that passes the filter settings
         */
        nap::Signal<const MidiEvent&> messageReceived;
        
        std::vector<MidiValue> mPorts; /**< Filter specifying input ports that will be listened to. Empty means all ports. */
        std::vector<MidiValue> mChannels; /**< Filter specifying what midi channels to listen to. Empty means all channels. */
        std::vector<MidiValue> mNumbers; /**< Filter specifying what number bytes (like cc numbers) to listen to. Empty means all numbers. */
        std::vector<MidiEvent::Type> mTypes; /**< Filter specifying what event types to listen to. Empty means all types. */

    protected:
        /**
         * This is triggered by the service when a new midi message is received
         */
        void trigger(const MidiEvent& event) { messageReceived(event); }
        
    private:
        MidiService* mService = nullptr;
    };
    
}