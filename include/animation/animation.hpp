#pragma once
#include "skeleton.hpp"
#include <string>
#include <memory>

namespace ve{
    
    class Animation{
        public:
            enum class PathType {
                TRANSLATION,
                ROTATION,
                SCALE
            };
            enum class InterpolationMethod{
                LINEAR,
                STEP
                // CUBICSPLINE
            };
            struct Channel {
                PathType pathType;       // "translation", "rotation", "scale"
                int samplerIndex; 
                int node;
            };
            struct Sampler{
                std::vector<float> timeStamps;
                std::vector<glm::vec4> TRSoutputValues;
                InterpolationMethod interpolationMethod;
            };
            Animation(std::string const& name);
            void start();
            void stop();
            bool isRunning() const;
            bool willExpire(const float& deltaTime )const; 
            void update(const float& deltaTime, Skeleton& skeleton);

            void setRepeat(bool repeat){isRepeat = repeat;}
            void setFirstKeyFrameTime(float frameTime){firstKeyFrameTime = frameTime;}
            void setLastKeyFrameTime(float frameTime){lastKeyFrameTime = frameTime;}
            float getDuration() const{return lastKeyFrameTime - firstKeyFrameTime;}
            float getCurrentTime() const{return currentKeyFrameTime - firstKeyFrameTime;}
            std::string const& getName() const{return name;}

            std::vector<Animation::Channel> channels;
            std::vector<Animation::Sampler> samplers;
        private:
            std::string name;
            bool isRepeat;
            float firstKeyFrameTime;
            float lastKeyFrameTime;
            float currentKeyFrameTime = 0.0f;
    };
}