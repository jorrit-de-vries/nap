#pragma once

// Audio includes
#include "audionode.h"
#include "audionodemanager.h"

namespace nap {
    
    namespace audio {
        
        /**
         * Node to play back audio from a buffer
         */
        class NAPAPI BufferPlayer : public Node
        {
        public:
            BufferPlayer(NodeManager& manager) : Node(manager) { }
        
            /**
             * The output to connect to other nodes
             */
            OutputPin audioOutput = { this  };
            
            /**
             * Tells the node to start playback
             * @param buffer: the buffer to play back from
             * @param position: the starting position in the source buffer in samples
             * @param speed: the playbackspeed, 1.0 means 1 sample per sample, 2 means double speed, etc.
             */
            void play(SampleBuffer& buffer, DiscreteTimeValue position = 0, ControllerValue speed = 1.);
            
            /**
             * Stops playback
             */
            void stop();
            
        private:
            void process() override;
  
            bool mPlaying = false;
            long double mPosition = 0;
            ControllerValue mSpeed = 1.f;
            SampleBufferPtr mBuffer = nullptr;
        };
        
    }
}