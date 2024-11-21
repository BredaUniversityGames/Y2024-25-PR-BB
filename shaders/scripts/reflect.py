import subprocess
import json
import vk_map
import glob
import os


# Description
# Traverses all the spv binary files we have in the shader/bin directory and creates json files with reflect
# data from them. This data is then cross-compared for similar data structures and vertex inputs and aggregates them.
# With that data we generate C++ header files that can be used in the project to have compile-time matching structures
# between the shaders and the C++ codebase.

# TODO: Properly output results to a C++ header
# TODO: Make sure to convert Vertex input data to structs
# TODO: Handle naming between different vertex inputs
# TODO: Proper errors if shaders cant properly convert to C++ code

class Field:
    def __init__(self, type, name):
        self.type = type
        self.name = name

    type = ""
    name = ""


# Generates the C++ syntax for a struct.d
def generate_struct(name, fields, file_name):
    content = f"#pragma once\n\n"
    content += f"struct {name} {{\n"

    for field in fields:
        content += f"    {field.type} {field.name};\n"

    content += "};\n"

    print(content)


# All spv files to process
spv_files = glob.glob('../bin' + '/**/*.spv', recursive=True)

# All the commands to execute
commands = []
for file in spv_files:
    commands.append(['spirv-cross', file, '--reflect'])

# The set of all types reflected from the spv files
final_types = {}

# Run all the reflection commands
for cmd in commands:
    result = subprocess.run(cmd, capture_output=True, text=True)

    print(cmd[1])
    if result.stderr:
        print(result.stderr)
        continue

    # Write the json file in the bin folder for debugging
    with open(cmd[1] + '.json', 'w') as file:
        file.write(result.stdout)

    shader_json = json.loads(result.stdout)

    # Generate vertex structs
    if ".vert" in cmd[1]:
        if 'inputs' in shader_json.keys():
            fields = []
            for input in shader_json['inputs']:
                fields.append(Field(vk_map.glsl_to_glm_map[input['type']], input['name']))

            # generate_struct("Vertex", fields, "vertex.hpp")

    if 'types' in shader_json:
        for index, (key, value) in enumerate(shader_json['types'].items()):
            print(value['name'])

            ubos = shader_json.get('ubos')

            # Check if type is a uniform block
            if ubos is not None and any(ubo.get('type') == key for ubo in ubos):
                continue

            # Check if type is a storage block
            ssbos = shader_json.get('ssbos')
            if ssbos is not None and any(ssbo.get('type') == key for ssbo in ssbos):
                continue

            # Check if type has offsets (used to remove stack equivalent types)
            if value['members'][0].get('offset') is None:
                continue

            # Check if type is built-in GLSL type
            if value['name'].startswith('gl_'):
                continue

            # Check if we already encountered this type before
            if value['name'] in final_types.keys():
                # Check if this type differs from the one we have stored
                if final_types[value['name']] != value:
                    # TODO: Properly handle naming duplicates
                    print("ERROR")
                    print(final_types[value['name']])
                    print(value)

            for member in value['members']:
                # Check if a member is a reference to a different structure
                if member['type'].startswith('_') and member['type'][1:].isdigit():
                    # Find type in the reflection data and change the type to the name instead
                    if any(at[0] == member['type'] for at in enumerate(shader_json['types'].items())):
                        member['type'] = member['name']

            final_types[value['name']] = value

print('\n\n')

for t in final_types:
    print(t)