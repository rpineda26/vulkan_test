#include "ve_camera.hpp"

#include <cassert>
#include <limits>

namespace ve {
    void VeCamera::setOrtho(float left, float right, float bottom, float top, float near, float far) {

        projectionMatrix = glm::mat4{1.0f};
        projectionMatrix[0][0] = 2.0f / (right - left);
        projectionMatrix[1][1] = 2.0f / (top - bottom);
        projectionMatrix[2][2] = 1.0f / (far - near);
        projectionMatrix[3][0] = -(right + left) / (right - left);
        projectionMatrix[3][1] = -(top + bottom) / (top - bottom);
        projectionMatrix[3][2] = -near / (far - near);
    }

    void VeCamera::setPerspective(float fovy, float aspect, float near, float far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovy = glm::tan(fovy / 2.0f);
        projectionMatrix = glm::mat4{0.0f};
        projectionMatrix[0][0] = 1.0f / (aspect * tanHalfFovy);
        projectionMatrix[1][1] = 1.0f / (tanHalfFovy);
        projectionMatrix[2][2] = far / (far - near);
        projectionMatrix[2][3] = 1.0f;
        projectionMatrix[3][2] = -(far * near) / (far - near);

    }
    void VeCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
        const glm::vec3 w = glm::normalize(direction);
        const glm::vec3 u = glm::normalize(glm::cross(w, up));
        const glm::vec3 v = glm::cross(w, u);

        viewMatrix = glm::mat4{1.0f};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
        //set inverse matrix
        inverseMatrix = glm::mat4{1.0f};
        inverseMatrix[0][0] = u.x;
        inverseMatrix[0][1] = u.y;
        inverseMatrix[0][2] = u.z;
        inverseMatrix[1][0] = v.x;
        inverseMatrix[1][1] = v.y;
        inverseMatrix[1][2] = v.z;
        inverseMatrix[2][0] = w.x;
        inverseMatrix[2][1] = w.y;
        inverseMatrix[2][2] = w.z;
        inverseMatrix[3][0] = position.x;
        inverseMatrix[3][1] = position.y;
        inverseMatrix[3][2] = position.z;
    }
    void VeCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        setViewDirection(position, target - position, up);
    }
    void VeCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        const float cosY = glm::cos(rotation.y);
        const float sinY = glm::sin(rotation.y);
        const float cosX = glm::cos(rotation.x);
        const float sinX = glm::sin(rotation.x);
        const float cosZ = glm::cos(rotation.z);
        const float sinZ = glm::sin(rotation.z);

        const glm::vec3 u{cosY * cosZ + sinY * sinX * sinZ, sinZ * cosX, cosY * sinX * sinZ - cosZ * sinY};
        const glm::vec3 v{cosZ * sinY * sinX - cosY * sinZ, cosX * cosZ, cosY * cosZ * sinX + sinY * sinZ};
        const glm::vec3 w{sinY * cosX, -sinX, cosY * cosX};

        viewMatrix = glm::mat4{1.0f};
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
        //set inverse matrix
        inverseMatrix = glm::mat4{1.0f};
        inverseMatrix[0][0] = u.x;
        inverseMatrix[0][1] = u.y;
        inverseMatrix[0][2] = u.z;
        inverseMatrix[1][0] = v.x;
        inverseMatrix[1][1] = v.y;
        inverseMatrix[1][2] = v.z;
        inverseMatrix[2][0] = w.x;
        inverseMatrix[2][1] = w.y;
        inverseMatrix[2][2] = w.z;
        inverseMatrix[3][0] = position.x;
        inverseMatrix[3][1] = position.y;
        inverseMatrix[3][2] = position.z;
    }
}