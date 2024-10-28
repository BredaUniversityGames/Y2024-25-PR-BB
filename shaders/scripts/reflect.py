import subprocess
import json
import vk_map
import glob
import os


class Field:
    def __init__(self, type, name):
        self.type = type
        self.name = name

    type = ""
    name = ""


def generate_struct(name, fields, file_name):
    content = f"#pragma once\n\n"
    content += f"struct {name} {{\n"

    for field in fields:
        content += f"    {field.type} {field.name};\n"

    content += "};\n"

    print(content)


spv_files = glob.glob('../bin' + '/**/*.spv', recursive=True)

commands = []
for file in spv_files:
    commands.append(['spirv-cross', file, '--reflect'])

finalTypes = {}

for cmd in commands:
    result = subprocess.run(cmd, capture_output=True, text=True)

    print(cmd[1])
    if result.stderr:
        print(result.stderr)
        continue

    with open(cmd[1] + '.json', 'w') as file:
        file.write(result.stdout)

    shaderJson = json.loads(result.stdout)

    # Generate vertex structs
    if ".vert" in cmd[1]:
        if 'inputs' in shaderJson.keys():
            fields = []
            for input in shaderJson['inputs']:
                fields.append(Field(vk_map.glsl_to_glm_map[input['type']], input['name']))

            # generate_struct("Vertex", fields, "vertex.hpp")

    if 'types' in shaderJson:
        for index, (key, value) in enumerate(shaderJson['types'].items()):
            print(value['name'])

            ubos = shaderJson.get('ubos')

            # Check if type is a uniform block
            if ubos is not None and any(ubo.get('type') == key for ubo in ubos):
                continue

            # Check if type is a storage block
            ssbos = shaderJson.get('ssbos')
            if ssbos is not None and any(ssbo.get('type') == key for ssbo in ssbos):
                continue

            # Check if type has offsets (used to remove stack equivalent types)
            if value['members'][0].get('offset') is None:
                continue

            # Check if type is built-in GLSL type
            if value['name'].startswith('gl_'):
                continue

            if value['name'] in finalTypes.keys():
                if finalTypes[value['name']] != value:
                    print("ERROR")
                    print(finalTypes[value['name']])
                    print(value)

            for member in value['members']:
                if member['type'].startswith('_') and member['type'][1:].isdigit():
                    if any(at[0] == member['type'] for at in enumerate(shaderJson['types'].items())):
                        member['type'] = member['name']
                        print(member)

            finalTypes[value['name']] = value

print('\n\n')

for t in finalTypes:
    print(t)
