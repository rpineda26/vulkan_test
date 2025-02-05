#include "ve_game_object.hpp"

#include <imgui.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ve {
    class SceneEditor {
        public:
            SceneEditor();
            void drawSceneEditor(VeGameObject::Map& gameObjects, glm::vec3& lightPosition);
            void drawObjectsColumn(VeGameObject::Map& gameObjects, glm::vec3& lightPosition);
            void drawProperties(VeGameObject::Map& gameObjects, glm::vec3& lightPosition);
        private:
            
            int selectedGameObject = -1;
    };
}