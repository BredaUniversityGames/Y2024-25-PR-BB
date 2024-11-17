#include <gtest/gtest.h>

#include "resource_manager.hpp"
#include <string>

template <>
std::weak_ptr<ResourceManager<std::string>> ResourceHandle<std::string>::manager = {};

TEST(ResourceManagerTests, Creating)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string original = "my_resource";
    std::string resource = original;

    // Act
    rm.Create(std::move(resource));

    // Assert
    EXPECT_GT(rm.Resources().size(), 0);
    EXPECT_EQ(rm.Resources().begin()->resource, original);
}

TEST(ResourceManagerTests, Accessing)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string original = "my_resource";
    std::string resource = original;

    // Act
    auto handle = rm.Create(std::move(resource));
    auto accessedResource = rm.Access(handle);

    // Assert
    EXPECT_FALSE(accessedResource == nullptr);
    EXPECT_EQ(*accessedResource, original);
}

TEST(ResourceManagerTests, Destroying)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string original = "my_resource";
    std::string resource = original;

    // Act
    auto handle = rm.Create(std::move(resource));
    rm.Destroy(handle);

    // Assert
    EXPECT_EQ(rm.Access(handle), nullptr);
}

TEST(ResourceManagerTests, Versioning)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string original1 = "my_resource1";
    std::string resource1 = original1;
    std::string original2 = "my_resource2";
    std::string resource2 = original2;

    // Act
    auto handle1 = rm.Create(std::move(resource1));
    rm.Destroy(handle1);

    auto handle2 = rm.Create(std::move(resource2));

    // Assert
    EXPECT_EQ(rm.Access(handle1), nullptr);
    EXPECT_EQ(*rm.Access(handle2), original2);
    EXPECT_EQ(rm.Resources().size(), 1);
}
