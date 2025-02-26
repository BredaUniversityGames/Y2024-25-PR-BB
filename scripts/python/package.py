import shutil
import sys
import os
import argparse
import json

def copy_dir(folder, dest):
    shutil.copytree(folder, dest, dirs_exist_ok=True)


def copy_file(file, dest_folder):
    shutil.copy(file, dest_folder)


def main():
    parser = argparse.ArgumentParser(description='Packages the project to /package/')
    parser.add_argument('-c', '--config', help="JSON config for packaging", type=str, required=True)

    args = parser.parse_args()
    config_file = open(args.config)
    config_data = json.load(config_file)

    output_dir = config_data['output']
    os.makedirs(output_dir, exist_ok=True)

    for executable in config_data['executables']:
        copy_file(executable, output_dir)

    for dir in config_data['assets']:
        copy_dir(dir, output_dir + "/" + dir)

    for file in config_data['files']:
        copy_file(file, output_dir)


if __name__ == "__main__":
    main()
