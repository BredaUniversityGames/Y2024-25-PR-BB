import os
import subprocess
import sys
from distutils.version import LooseVersion

# Set the base directory to search for shader files
base_dir = os.path.dirname(os.path.realpath(__file__))

# Find the latest version of VulkanSDK in the C:/VulkanSDK/ directory
vulkan_sdk_base = "C:/VulkanSDK/"
available_versions = [d for d in os.listdir(vulkan_sdk_base) if os.path.isdir(os.path.join(vulkan_sdk_base, d))]

if not available_versions:
    print("No VulkanSDK versions found.")
    sys.exit(1)

latest_version = max(available_versions, key=LooseVersion)
glslc_path = os.path.join(vulkan_sdk_base, latest_version, "Bin", "glslc.exe")

if not os.path.exists(glslc_path):
    print(f"glslc compiler not found at {glslc_path}.")
    sys.exit(1)

# List of shader extensions and their corresponding output suffixes
shader_types = {
    ".frag": "-f.spv",
    ".vert": "-v.spv",
    ".geom": "-g.spv",
    ".tesc": "-tc.spv",
    ".tese": "-te.spv",
    ".comp": "-c.spv",
}

# Function to compile a shader file
def compile_shader(shader_path, output_path):
    command = [glslc_path, shader_path, "-o", output_path]
    try:
        subprocess.run(command, check=True)
        print(f"Successfully compiled {shader_path} to {output_path}")
    except subprocess.CalledProcessError as e:
        print(f"Error compiling {shader_path}: {e}")

# Recursively search for shader files and compile them
for root, _, files in os.walk(base_dir):
    for file in files:
        for extension, suffix in shader_types.items():
            if file.endswith(extension):
                shader_path = os.path.join(root, file)
                output_path = os.path.join(root, file.replace(extension, suffix))
                compile_shader(shader_path, output_path)
                break
