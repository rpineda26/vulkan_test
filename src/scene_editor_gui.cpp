#include "scene_editor_gui.hpp"
#include <limits.h>
#include <stdio.h>

namespace ve{
    SceneEditor::SceneEditor(){
        
    }

    void SceneEditor::drawSceneEditor(VeGameObject::Map& gameObjects, glm::vec3& lightPosition){
        ImGui::Begin("Scene Editor");
        ImGui::Columns( 2);
        // Game Objects Section
        if (ImGui::CollapsingHeader("Game Objects")) {
            drawObjectsColumn(gameObjects, lightPosition);

        }
        char label[26];
        snprintf(label, sizeof(label), "Light##%ld", gameObjects.size());
        if(ImGui::Selectable(label, selectedGameObject == gameObjects.size())){
            selectedGameObject = gameObjects.size();
        }
        ImGui::NextColumn();
        if(selectedGameObject != -1){
            drawProperties(gameObjects, lightPosition);
        }
        else{
            ImGui::Text("Select an object to view properties");
        }


        ImGui::End();
    }
    void SceneEditor::drawObjectsColumn(VeGameObject::Map& gameObjects, glm::vec3& lightPosition){
         for (auto& [id, object] : gameObjects) {
            char label[26];
            snprintf(label, sizeof(label), "%s##%d", object.getTitle(), id);
            if (strcmp(object.getTitle(),"")==0) snprintf(label, sizeof(label), "Object %d##%d", id, id);
            
            if (ImGui::Selectable(label, selectedGameObject == id)) {
                selectedGameObject = id;
            }
        }
    }
    void SceneEditor::drawProperties(VeGameObject::Map& gameObjects, glm::vec3& lightPosition){
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
            }
        }
        if(selectedGameObject == gameObjects.size()){
            ImGui::Text("Light Position");
            ImGui::SameLine();
            ImGui::InputFloat3("##LightPosition", &lightPosition.x);
        }

    }
}