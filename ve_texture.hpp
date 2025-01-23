#pragma once
#include "ve_device.hpp"

#include <vulkan/vulkan.h>

#include <string>
namespace ve{
    class VeTexture{
        public:
            VeTexture(VeDevice& device, const std::string& albedoPath, const std::string& normalPath); 
            ~VeTexture();
            VeTexture(const VeTexture&) = delete;
            VeTexture& operator=(const VeTexture&) = delete;
            VeTexture(VeTexture&&) = delete;
            VeTexture& operator=(VeTexture&&) = delete;

            VkImageView getImageView() const { return textureImageView; }
            VkSampler getSampler() const { return textureSampler; }
            VkImageView getNormalImageView() const { return normalImageView; }
            VkSampler getNormalSampler() const { return normalSampler; }

            VkImageLayout getLayout() const { return textureLayout; } // same for both albedo and normal
        private:
            void createTextureImage(VkFormat textureFormat, const std::string& path, VkImage& image, VkDeviceMemory& imageMemory, VkImageView& imageView, VkSampler& imageSampler);
            void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, int mipLevels, VkImage image);
            void generateMipMaps(VkFormat textureFormat, VkImage image, int texWidth, int texHeight, int mipLevels);
            VeDevice& veDevice;
            //albedo texture map
            VkImage textureImage;
            VkDeviceMemory textureImageMemory;
            VkSampler textureSampler;
            VkImageView textureImageView;
            //normal map
            VkImage normalImage;
            VkDeviceMemory normalImageMemory;
            VkSampler normalSampler;
            VkImageView normalImageView;

            VkImageLayout textureLayout; //same for both albedo and normal
    };
}