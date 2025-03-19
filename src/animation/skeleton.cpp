#include "skeleton.hpp"
#include <iostream>
namespace ve{
    Skeleton::Skeleton(){}
    Skeleton::~Skeleton(){}
    void Skeleton::traverse(){
        int indent = 0;
        std::cout << "Skeleton indent: " << name << std::endl;
        auto& root = joints[0];
        traverse(root, indent + 1);
    }
    void Skeleton::traverse(Joint const& joint, int indent){
        size_t numChildren = joint.childrenIndices.size();
        for(size_t i = 0; i < numChildren; i++){
            auto& child = joints[joint.childrenIndices[i]];
            std::cout << std::string(indent, ' ') << "Skeleton " << joint.childrenIndices[i] << ": " << child.name << std::endl;
            traverse(child, indent + 1);
        }
    }
    //update the coordinates of all joints based on animation
    void Skeleton::update(){
        int16_t numJoints = static_cast<int16_t>(joints.size());
        if(isAnimated){
            //apply animation transform
            for(int16_t i = 0; i < numJoints; i++){
                jointMatrices[i] = joints[i].getAnimatedMatrix();
            }
            //recursively update joint matrices
            updateJoint(ROOT_JOINT);
            //return from animated space to original model space
            for(int16_t i = 0; i < numJoints; i++){
                jointMatrices[i] = jointMatrices[i] * joints[i].inverseBindMatrix;
            }
        }else{
            for(int16_t i=0; i<numJoints;i++){
                jointMatrices[i] = glm::mat4(1.0f);
            }
        }
    }
    void Skeleton::updateJoint(int16_t jointIndex){
        auto& currentJoint = joints[jointIndex];
        int16_t parentJoint = currentJoint.parentIndex;
        if(parentJoint != NO_PARENT){
            jointMatrices[jointIndex] = jointMatrices[parentJoint] * jointMatrices[jointIndex];
        }
        size_t numChildren = currentJoint.childrenIndices.size();
        for(size_t i = 0; i < numChildren; i++){
            updateJoint(currentJoint.childrenIndices[i]);
        }
    }
}