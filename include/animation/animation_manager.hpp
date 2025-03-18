#pragma once
#include "animation.hpp"
#include <map>
#include <memory>

namespace ve{
    class AnimationManager{
        public: 
            using pAnimation = std::shared_ptr<Animation>;
            struct Iterator{
                Iterator(pAnimation* ptr): ptr(ptr){}
                Iterator& operator++(){
                    ptr++;
                    return *this;
                }
                Animation& operator*(){
                    return **ptr;
                }
                bool operator!=(const Iterator& other){
                    return ptr != other.ptr;
                }
                private:
                    pAnimation* ptr;
            };

        public:
            Iterator begin();
            Iterator end();
            Animation& operator[](std::string const& animation);
            Animation& operator[](int index);

        public:
            AnimationManager();
            size_t size() const {return animationsMap.size();}
            void push(pAnimation const& animation);
            void start(std::string const& animation);
            void start(size_t index);
            void start(){start(0);};
            void stop();
            void update(const float& deltaTime, Skeleton& skeleton, int frameCounter);
            bool isRunning() const;
            bool willExpire(const float& deltaTime) const;
            void setRepeat(bool repeat);
            void setRepeatAll(bool repeat);
            float getDuration(std::string const& animation);
            float getCurrentTime();
            std::string getName();
            int getIndex(std::string const& animation);

        private:
            std::map<std::string, std::shared_ptr<Animation>> animationsMap;
            std::vector<std::shared_ptr<Animation>> animationsVector;
            Animation* currentAnimation;
            int frameCount;
            std::map<std::string, int> nameIndexMap;
    };
}