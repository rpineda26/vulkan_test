#include "scene_editor_gui.hpp"
#include <limits.h>
#include <stdio.h>

namespace ve{
    SceneEditor::SceneEditor(){
        
    }

    void SceneEditor::drawSceneEditor(VeGameObject::Map& gameObjects, int& selectedObject, VeGameObject& camera){
        ImGui::Begin("Scene Editor");
        ImGui::Columns( 2);
        // Game Objects Section
        if (ImGui::CollapsingHeader("Game Objects")) {
            drawObjectsColumn(gameObjects, selectedObject, false);
        }
        // Light Objects Section
        if(ImGui::CollapsingHeader("Point Lights")){
            drawObjectsColumn(gameObjects, selectedObject, true);
        }
        // Camera
        // drawCameraSelectable();
        char label[26];
        snprintf(label, sizeof(label), "Camera##%d", -1);
        if(ImGui::Selectable(label, selectedGameObject == -1)){
            selectedGameObject = -1;
            selectedObject = -1;
        }
        ImGui::NextColumn();
    
        drawProperties(gameObjects, camera);



        ImGui::End();
    }
    void SceneEditor::drawObjectsColumn(VeGameObject::Map& gameObjects, int& selectedObject, bool isLight){
         for (auto& [id, object] : gameObjects) {
            if(isLight && object.lightComponent!=nullptr){    
                char label[26];
                snprintf(label, sizeof(label), "%s##%d", object.getTitle(), id);
                if (strcmp(object.getTitle(),"")==0) snprintf(label, sizeof(label), "Object %d##%d", id, id);
                if (ImGui::Selectable(label, selectedGameObject == id)) {
                    selectedGameObject = id;
                    selectedObject = id;
                }
            }else if(!isLight && object.lightComponent==nullptr){
                char label[26];
                snprintf(label, sizeof(label), "%s##%d", object.getTitle(), id);
                if (strcmp(object.getTitle(),"")==0) snprintf(label, sizeof(label), "Object %d##%d", id, id);
                if (ImGui::Selectable(label, selectedGameObject == id)) {
                    selectedGameObject = id;
                    selectedObject = id;
                }
            }
        }
    }
    void SceneEditor::drawProperties(VeGameObject::Map& gameObjects, VeGameObject& camera){
        if(selectedGameObject == -1){
            ImGui::Text("Properties");      
            // Position
            if(ImGui::CollapsingHeader("Physical Attributes")){
                ImGui::Text("Position");
                ImGui::SameLine();
                ImGui::InputFloat3("##Position", &camera.transform.translation.x);
            // Rotation
                ImGui::Text("Rotation ");
                ImGui::SameLine();
                ImGui::InputFloat3("##Rotation", &camera.transform.rotation.x);
            // Scale
                ImGui::Text("Scale");
                ImGui::SameLine();
                ImGui::InputFloat3("##Scale", &camera.transform.scale.x);
            }
        }
        for(auto& [id, object] : gameObjects){
            if(id == selectedGameObject){
                ImGui::Text("Properties");
                ImGui::Separator();
                ImGui::Text("Name: ");
                ImGui::SameLine();
                ImGui::InputText("##Name", object.getTitle(), sizeof(object.getTitle()));
                // Position
                if(ImGui::CollapsingHeader("Physical Attributes")){
                    ImGui::Text("Position");
                    ImGui::SameLine();
                    ImGui::InputFloat3("##Position", &object.transform.translation.x);
                // Rotation
                    ImGui::Text("Rotation ");
                    ImGui::SameLine();
                    ImGui::InputFloat3("##Rotation", &object.transform.rotation.x);
                // Scale
                    ImGui::Text("Scale");
                    ImGui::SameLine();
                    ImGui::InputFloat3("##Scale", &object.transform.scale.x);
                    // Color
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##Color", &object.color.x);
                }
                //Texture
                if(object.lightComponent==nullptr){
                    
                    ImGui::Text("Texture Index");
                    ImGui::SameLine();
                    int textureIndex = object.model->getTextureIndex();
                    ImGui::InputInt("##TextureIndex", &textureIndex);
                    object.model->setTextureIndex(textureIndex%5);
                    //Normal Map
                    ImGui::Text("Normal Map Index");
                    ImGui::SameLine();
                    int normalIndex = object.model->getNormalIndex();
                    ImGui::InputInt("##NormalIndex", &normalIndex);
                    object.model->setNormalIndex(normalIndex%5);
                    int specularIndex = object.model->getSpecularIndex();
                    ImGui::Text("Specular Map Index");
                    ImGui::SameLine();
                    ImGui::InputInt("##SpecularIndex", &specularIndex);
                    object.model->setSpecularIndex(specularIndex%5);
                    ImGui::Text("Smoothness");
                    ImGui::SameLine();
                    float smoothness = object.model->getSmoothness();
                    ImGui::SliderFloat("##Smoothness", 
                        &smoothness,  // Pointer to the value
                        0.0f,                       // Minimum value
                        1.0f,                       // Maximum value
                        "%.2f",                     // Format string (shows 2 decimal places)
                        ImGuiSliderFlags_None       // Optional flags
                    );
                    object.model->setSmoothness(smoothness);
                }else{
                    ImGui::Text("Light Intensity");
                    ImGui::SameLine();
                    float lightIntensity = object.lightComponent->lightIntensity;
                    ImGui::SliderFloat("##LightIntensity", 
                        &lightIntensity,  // Pointer to the value
                        0.0f,                       // Minimum value
                        10.0f,                       // Maximum value
                        "%.2f",                     // Format string (shows 2 decimal places)
                        ImGuiSliderFlags_None       // Optional flags
                    );
                    object.lightComponent->lightIntensity = lightIntensity;
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::InputFloat("##Radius", &object.transform.scale.x);
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##Color", &object.color.x);
                    
                }
            }
        }

    }
}