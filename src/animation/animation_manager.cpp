#include "animation_manager.hpp"
#include <iostream>
namespace ve{
    AnimationManager::AnimationManager(): currentAnimation(nullptr), frameCount(0){}
    Animation& AnimationManager::operator[](std::string const& animation){
        return *animationsMap[animation];
    }
    Animation& AnimationManager::operator[](int index){
        return *animationsVector[index];
    }
    void AnimationManager::push(pAnimation const& animation){
        if(animation){
            animationsMap.emplace(animation->getName(), animation);
            animationsVector.push_back(animation);
            nameIndexMap[animation->getName()] = animationsVector.size() - 1;
        }else{
            std::cout << "Animation is null" << std::endl;
        }
    }
    void AnimationManager::start(std::string const& animation){
        Animation* currAnim= animationsMap[animation].get();
        if(currAnim){
            currentAnimation = currAnim;
            currentAnimation->start();
        }
    }
    void AnimationManager::start(size_t index){
        if(!(index < animationsVector.size())){
            std::cout<< "Animation start index out of range" << std::endl;
            return;
        }
        Animation* currAnim = animationsVector[index].get();
        if(currAnim){
            currentAnimation = currAnim;
            currentAnimation->start();
        }
    }
    void AnimationManager::stop(){
        if(currentAnimation){
            currentAnimation->stop();
        }
    }
    void AnimationManager::update(const float& deltaTime, ve::Skeleton& skeleton, int frameCounter){
        // std::cout << "AnimationManager update" << std::endl;
        if(frameCount != frameCounter){
            // std::cout << "AnimationManager udpdate condition reachable" <<std::endl;
            frameCount = frameCounter;
            if(currentAnimation){
                // std::cout<<"updating "<<currentAnimation->getName()<<std::endl;
                currentAnimation->update(deltaTime, skeleton);
            }
        }
    }
    bool AnimationManager::isRunning() const{
        if(currentAnimation){
            return currentAnimation->isRunning();
        }
        return false;
    }
    bool AnimationManager::willExpire(const float& deltaTime) const{
        if(currentAnimation){
            return currentAnimation->willExpire(deltaTime);
        }
        return false;
    }
    void AnimationManager::setRepeat(bool repeat){
        if(currentAnimation){
            currentAnimation->setRepeat(repeat);
        }
    }
    void AnimationManager::setRepeatAll(bool repeat){
        for(auto& animation: animationsVector){
            animation->setRepeat(repeat);
        }
    }
    float AnimationManager::getCurrentTime(){
        if(currentAnimation){
            return currentAnimation->getCurrentTime();
        }
        return 0.0f;
    }
    float AnimationManager::getDuration(std::string const& animation){
        return animationsMap[animation]->getDuration();
    }
    std::string AnimationManager::getName(){
        if(currentAnimation){
            return currentAnimation->getName();
        }
        return "";
    }
    int AnimationManager::getIndex(std::string const& animation){
        bool found = false;
        for (auto& element : animationsVector){
            if (element->getName() == animation){
                found = true;
                break;
            }
        }
        if(found)
            return nameIndexMap[animation];
        return -1;
    }

    AnimationManager::Iterator AnimationManager::begin(){
        return Iterator(&(*animationsVector.begin()));
    }
    AnimationManager::Iterator AnimationManager::end(){
        return Iterator(&(*animationsVector.end()));
    }
}