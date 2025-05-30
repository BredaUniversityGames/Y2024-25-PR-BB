#pragma once
#include <cereal/cereal.hpp>
#include <resource_management/image_resource_manager.hpp>
#include <cereal/types/string.hpp>

template <typename T>
struct HandleWrapper
{
    std::string resourceName;
    ResourceHandle<T> handle;
};



class UIResources
{
public:
    UIResources(ImageResourceManager& imageResourceManager,  ResourceHandle<Sampler> defaultSampler) : resourceManager(imageResourceManager), _defaultSampler(defaultSampler)
{}
    HandleWrapper<GPUImage> Create(std::string_view path)
    {
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;
        try{
        commonImageData.FromPNG(path);
        }
        catch(std::exception& e)
        {
            bblog::warn("Failed to load image: {}", e.what());
        }
        
        return pathToResourceMap[std::string(path)] = HandleWrapper<GPUImage>{std::string(path), resourceManager.Create(commonImageData.FromPNG(path))};

        }

    HandleWrapper<GPUImage> Get(std::string_view path)
    {
        if(!pathToResourceMap.contains(std::string(path)))
        {
            return Create(path);   
        }
        
        return pathToResourceMap.at(std::string(path));
    }
    
private:
    ResourceHandle<Sampler> _defaultSampler; 
    ImageResourceManager& resourceManager;
    std::unordered_map<std::string,HandleWrapper<GPUImage>> pathToResourceMap;

};

class UISerializationContext
{
public:
    static UIResources* uiResources;
};

template<typename Archive,typename T>
void save(Archive& ar,  const HandleWrapper<T>& h)
{
    ar(CEREAL_NVP(h.resourceName));
}

template<typename Archive,typename T>
void load(Archive& ar,  HandleWrapper<T>& h)
{
    ar(CEREAL_NVP(h.resourceName));
    h.handle = UISerializationContext::uiResources->Get(h.resourceName).handle;
}
