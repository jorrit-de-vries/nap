#include "audioobject.h"


// Nap includes
#include <nap/entity.h>
#include <nap/core.h>

// Audio includes
#include <service/audioservice.h>


// RTTI
RTTI_DEFINE_BASE(nap::audio::AudioObject)
RTTI_DEFINE_BASE(nap::audio::AudioObjectInstance)
RTTI_DEFINE_BASE(nap::audio::MultiChannelObject)

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::audio::MultiChannelObjectInstance)
    RTTI_FUNCTION("getNode", &nap::audio::MultiChannelObjectInstance::getNode)
RTTI_END_CLASS


namespace nap {
    
    namespace audio {
        
        
        std::unique_ptr<AudioObjectInstance> AudioObject::instantiate(NodeManager& nodeManager, utility::ErrorState& errorState)
        {
            auto instance = createInstance();
            mInstance = instance.get();
            if (!instance->init(nodeManager, errorState))
                return nullptr;
            else
                return instance;
        }

        
        std::unique_ptr<AudioObjectInstance> MultiChannelObject::createInstance()
        {
            return std::make_unique<MultiChannelObjectInstance>(*this);            
        }
        
        
        bool MultiChannelObjectInstance::init(NodeManager& nodeManager, utility::ErrorState& errorState)
        {
            auto resource = rtti_cast<MultiChannelObject>(&getResource());
            for (auto channel = 0; channel < resource->getChannelCount(); ++channel)
            {
                auto node = resource->createNode(channel, nodeManager);
                assert(node->getOutputs().size() == 1);
                mNodes.emplace_back(std::move(node));
            }
            return true;
        }

        
        Node* MultiChannelObjectInstance::getNode(int channel)
        {
            if (channel < mNodes.size())
                return mNodes[channel].get();
            else
                return nullptr;
        }


    
    }
    
}
