#include "ve_game_object.hpp"

#include <imgui.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ve {
    class SceneEditor {
        public:
            SceneEditor();
            void drawSceneEditor(VeGameObject::Map& gameObjects, int& selectedObject, VeGameObject& camera, int& numLights, bool& isOutlignHighlight);
            void drawObjectsColumn(VeGameObject::Map& gameObjects,int& selectedObject, bool isLight);
            void drawProperties(VeGameObject::Map& gameObjects, VeGameObject& camera);
            void addObject(VeGameObject::Map& gameObjects, int& numLights, int& selectedObject);
            void selectModel(VeGameObject::Map& gameObjects, VeGameObject& object);
        private:
            
            int selectedGameObject = -1;
            bool showObjectOptions = false;
    };
}