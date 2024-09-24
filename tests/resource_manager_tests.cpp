#include <gtest/gtest.h>

#include "resource_manager.hpp"
#include <string>

TEST(ResourceManagerTests, Creating)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string resource = "my_resource";

    // Act
    auto handle = rm.Create(resource);

    // Assert
    EXPECT_GT(rm.Resources().size(), 0);
    EXPECT_EQ(rm.Resources().begin()->resource, resource);
}

TEST(ResourceManagerTests, Accessing)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string resource = "my_resource";

    // Act
    auto handle = rm.Create(resource);
    auto accessedResource = rm.Access(handle);

    // Assert
    EXPECT_FALSE(accessedResource == nullptr);
    EXPECT_EQ(*accessedResource, resource);
}

TEST(ResourceManagerTests, Destroying)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string resource = "my_resource";

    // Act
    auto handle = rm.Create(resource);
    rm.Destroy(handle);

    // Assert
    EXPECT_EQ(rm.Access(handle), nullptr);
}

TEST(ResourceManagerTests, Versioning)
{
    // Arrange
    ResourceManager<std::string> rm;
    std::string resource1 = "my_resource1";
    std::string resource2 = "my_resource2";

    // Act
    auto handle1 = rm.Create(resource1);
    rm.Destroy(handle1);

    auto handle2 = rm.Create(resource2);

    // Assert
    EXPECT_EQ(rm.Access(handle1), nullptr);
    EXPECT_EQ(*rm.Access(handle2), resource2);
    EXPECT_EQ(rm.Resources().size(), 1);
}
