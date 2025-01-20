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

        const glm::vec3 xaxis = {cosY * cosZ + sinY * sinX * sinZ, sinZ * cosX, -sinY * cosZ + cosY * sinX * sinZ};
        const glm::vec3 yaxis = {cosY * sinZ - sinY * sinX * cosZ, cosZ * cosX, -sinY * sinZ - cosY * sinX * cosZ};
        const glm::vec3 zaxis = {sinY * cosX, -sinX, cosY * cosX};

        viewMatrix = glm::mat4{1.0f};
        viewMatrix[0][0] = xaxis.x;
        viewMatrix[1][0] = xaxis.y;
        viewMatrix[2][0] = xaxis.z;
        viewMatrix[0][1] = yaxis.x;
        viewMatrix[1][1] = yaxis.y;
        viewMatrix[2][1] = yaxis.z;
        viewMatrix[0][2] = zaxis.x;
        viewMatrix[1][2] = zaxis.y;
        viewMatrix[2][2] = zaxis.z;
        viewMatrix[3][0] = -glm::dot(xaxis, position);
        viewMatrix[3][1] = -glm::dot(yaxis, position);
        viewMatrix[3][2] = -glm::dot(zaxis, position);
    }
}