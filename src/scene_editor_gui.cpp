#include "scene_editor_gui.hpp"
#include "ve_model.hpp"
#include "utility.hpp"
#include <limits.h>
#include <stdio.h>

namespace ve{
    SceneEditor::SceneEditor(){
        
    }

    void SceneEditor::drawSceneEditor(VeGameObject::Map& gameObjects, int& selectedObject, VeGameObject& camera, int& numLights){
        ImGui::Begin("Scene Editor");
        addObject(gameObjects, numLights, selectedObject);
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
                if(object.lightComponent==nullptr){
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
                    }
                    //Texture
                    if(ImGui::CollapsingHeader("Textures")){
                        ImGui::Text("Albedo Index");
                        int textureIndex = object.getTextureIndex();
                        ImGui::InputInt("##TextureIndex", &textureIndex);
                        object.setTextureIndex(textureIndex%5);
                        // Color
                        ImGui::Text("Color");
                        ImGui::SameLine();
                        ImGui::ColorEdit3("##Color", &object.color.x);
                        if(ImGui::Button("Reset Color")){
                            object.color = glm::vec3(1.0);
                        }
                        
                        //Normal Map
                        ImGui::Text("Normal Map Index");
                        ImGui::SameLine();
                        int normalIndex = object.getNormalIndex();
                        ImGui::InputInt("##NormalIndex", &normalIndex);
                        object.setNormalIndex(normalIndex%5);
                        int specularIndex = object.getSpecularIndex();
                        ImGui::Text("Specular Map Index");
                        ImGui::SameLine();
                        ImGui::InputInt("##SpecularIndex", &specularIndex);
                        object.setSpecularIndex(specularIndex%5);
                        ImGui::Text("Smoothness");
                        ImGui::SameLine();
                        float smoothness = object.getSmoothness();
                        ImGui::SliderFloat("##Smoothness", 
                            &smoothness, 
                            0.0f,                 
                            1.0f,                 
                            "%.2f",             
                            ImGuiSliderFlags_None 
                        );
                        object.setSmoothness(smoothness);
                    }
                    if(ImGui::Button("Select Model")){
                        ImGui::OpenPopup("Model Selection##replace");
                    }
                    if(ImGui::BeginPopup("Model Selection##replace")){
                        selectModel(gameObjects, object);
                    }
                }else{
                    ImGui::Text("Position");
                    ImGui::SameLine();
                    ImGui::InputFloat3("##Position", &object.transform.translation.x);
                    ImGui::Text("Rotation ");
                    ImGui::SameLine();
                    ImGui::InputFloat3("##Rotation", &object.transform.rotation.x);
                    ImGui::Text("Light Intensity");
                    ImGui::SameLine();
                    float lightIntensity = object.lightComponent->lightIntensity;
                    ImGui::SliderFloat("##LightIntensity", 
                        &lightIntensity,  
                        0.0f,                       
                        20.0f,                      
                        "%.2f",                   
                        ImGuiSliderFlags_None       
                    );
                    object.lightComponent->lightIntensity = lightIntensity;
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::InputFloat("##Radius", &object.transform.scale.x);
                    ImGui::Text("Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit3("##Color", &object.color.x);
                    if(ImGui::Button("Reset Color")){
                        object.color = glm::vec3(1.0);
                    }
                }
            }
        }

    }
    void SceneEditor::addObject(VeGameObject::Map& gameObjects, int& numLights, int& selectedObject){
        if(ImGui::Button("Add Object")){
            showObjectOptions = true;
        }
        if(showObjectOptions){
            ImGui::Text("Select Object Type");
            ImGui::BeginDisabled(numLights>=10);
            if(ImGui::Button("Point Light")){
                if(numLights<10){
                    VeGameObject light = VeGameObject::createPointLight(1.0f, 0.1f, {1.0f,1.0f,1.0f});
                    char* title = new char[26];
                    snprintf(title, sizeof(title), "Light %d", light.getId());
                    light.setTitle(title);
                    gameObjects.emplace(light.getId(),std::move(light));
                    selectedGameObject = light.getId();
                    selectedObject = light.getId();
                    numLights++;
                }
                showObjectOptions = false;
            }
            ImGui::EndDisabled();
            if(ImGui::Button("Sample Models")){
                ImGui::OpenPopup("Model Selection##new");
            }
            if(ImGui::BeginPopup("Model Selection##new")){
                ImGui::Text("Select a model: ");
                for(const auto& model: modelFileNames){
                    if(ImGui::Selectable(model.c_str())){
                        auto object = VeGameObject::createGameObject();
                        object.setTextureIndex(0);
                        object.setNormalIndex(0);
                        object.setSpecularIndex(0);
                        object.model = preLoadedModels[model];
                        char* title = new char[26];
                        snprintf(title, sizeof(title), "%s %d", model.c_str(), object.getId());
                        object.setTitle(title);
                        gameObjects.emplace(object.getId(),std::move(object));
                        selectedGameObject = object.getId();
                        selectedObject = object.getId();
                        showObjectOptions = false;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }
    }
    void SceneEditor::selectModel(VeGameObject::Map& gameObjects, VeGameObject& object){
       ImGui::Text("Select a model: ");
        for(const auto& model: modelFileNames){
            if(ImGui::Selectable(model.c_str())){
                object.model = preLoadedModels[model];
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}