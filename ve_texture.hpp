#pragma once
#include "ve_device.hpp"

#include <vulkan/vulkan.h>

#include <string>
namespace ve{
    class VeTexture{
        public:
            VeTexture(VeDevice& device, const std::string& filePath);
            ~VeTexture();
            VeTexture(const VeTexture&) = delete;
            VeTexture& operator=(const VeTexture&) = delete;
            VeTexture(VeTexture&&) = delete;
            VeTexture& operator=(VeTexture&&) = delete;

            VkImageView getImageView() const { return textureImageView; }
            VkSampler getSampler() const { return textureSampler; }
            VkImageLayout getLayout() const { return textureLayout; }
           
        private:
            void createTextureImage();
            void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
            void generateMipMaps();
            VeDevice& veDevice;
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkSampler textureSampler;
            VkImageView textureImageView;
            VkFormat textureFormat;
            VkImageLayout textureLayout;
            std::string filePath;
            int texWidth, texHeight, texChannels, mipLevels;
    };
}