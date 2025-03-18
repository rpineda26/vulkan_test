#include "animation.hpp"

namespace ve{
    Animation::Animation(std::string const& name): name(name), isRepeat(false){}
    void Animation::start(){
        currentKeyFrameTime = firstKeyFrameTime;
    }
    void Animation::stop(){
        currentKeyFrameTime = lastKeyFrameTime + 1.0f;
    }
    bool Animation::isRunning() const{
        return isRepeat || (currentKeyFrameTime <= lastKeyFrameTime);
    }
    bool Animation::willExpire(const float& deltaTime) const{
        return !isRepeat || (currentKeyFrameTime + deltaTime > lastKeyFrameTime);
    }
    void Animation::update(const float& deltaTime, ve::Skeleton& skeleton){
        if(!isRunning()) 
            return;
        currentKeyFrameTime += deltaTime;
        if(isRepeat && (currentKeyFrameTime > lastKeyFrameTime)){
            if(isRepeat){
                currentKeyFrameTime = firstKeyFrameTime;
            }else{
                currentKeyFrameTime = lastKeyFrameTime + 1.0f;
            }
        }
        for(auto& channel: channels){
            auto& sampler = samplers[channel.samplerIndex];
            int jointIndex = skeleton.nodeJointMap[channel.node];
            auto& joint = skeleton.joints[jointIndex];
            //find the keyframe
            for(size_t i = 0; i < sampler.timeStamps.size() - 1; i++){
                if(currentKeyFrameTime>=sampler.timeStamps[i] && currentKeyFrameTime <= sampler.timeStamps[i+1]){
                    switch (sampler.interpolationMethod){
                        case InterpolationMethod::LINEAR:{                 
                            float t = (currentKeyFrameTime - sampler.timeStamps[i]) / (sampler.timeStamps[i + 1] - sampler.timeStamps[i]);
                            switch (channel.pathType){
                                case PathType::TRANSLATION:
                                    joint.translation = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                                case PathType::ROTATION:
                                    glm::quat q1;
                                    q1.x = sampler.TRSoutputValues[i].x;
                                    q1.y = sampler.TRSoutputValues[i].y;
                                    q1.z = sampler.TRSoutputValues[i].z;
                                    q1.w = sampler.TRSoutputValues[i].w;
                                    glm::quat q2;
                                    q2.x = sampler.TRSoutputValues[i+1].x;
                                    q2.y = sampler.TRSoutputValues[i+1].y;
                                    q2.z = sampler.TRSoutputValues[i+1].z;
                                    q2.w = sampler.TRSoutputValues[i+1].w;
                                    joint.rotation = glm::normalize(glm::slerp(q1, q2, t));
                                    break;
                                case PathType::SCALE:
                                    joint.scale = glm::mix(sampler.TRSoutputValues[i], sampler.TRSoutputValues[i+1], t);
                                    break;
                            }
                            break;
                        }
                        case InterpolationMethod::STEP:{
                            switch (channel.pathType){
                                case PathType::TRANSLATION:{
                                    skeleton.joints[channel.node].translation = sampler.TRSoutputValues[i];
                                    break;
                                }
                                case PathType::ROTATION:{
                                    skeleton.joints[channel.node].rotation.x = sampler.TRSoutputValues[i].x;
                                    skeleton.joints[channel.node].rotation.y = sampler.TRSoutputValues[i].y;
                                    skeleton.joints[channel.node].rotation.z = sampler.TRSoutputValues[i].z;
                                    skeleton.joints[channel.node].rotation.w = sampler.TRSoutputValues[i].w;
                                    break;
                                }
                                case PathType::SCALE:{
                                    skeleton.joints[channel.node].scale = sampler.TRSoutputValues[i];
                                    break;
                                }
                            }
                            break;
                        }
                            
                    }
                }
            }
        }

    }
}