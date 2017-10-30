#pragma once

#include <rtti/rttiobject.h>
#include <utility/dllexport.h>

#include <RtMidi.h>

#include "midiservice.h"

namespace nap {
    
    class NAPAPI MidiInputPort : public rtti::RTTIObject {
        RTTI_ENABLE(rtti::RTTIObject)
        
    public:
        MidiInputPort() = default;
        MidiInputPort(MidiService& service);
        virtual ~MidiInputPort();
        
        bool init(utility::ErrorState& errorState) override;
        
        MidiService& getService() { return *mService; }
        
        int mPortNumber = 0;
        bool mDebugOutput = false;
        
        void receiveEvent(std::unique_ptr<MidiEvent> event) { mService->enqueueEvent(std::move(event)); }
        
    private:
        RtMidiIn midiIn;
        MidiService* mService = nullptr;
    };
    
    // Object creator used for constructing the the OSC receiver
    using MidiInputPortObjectCreator = rtti::ObjectCreator<MidiInputPort, MidiService>;
    
}
