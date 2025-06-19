import shutil
import sys
import os
import argparse
import json
import stat
import zipfile


def zip_directories(output_zip, directories):
    with zipfile.ZipFile(output_zip, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for dir_path in directories:
            for root, _, files in os.walk(dir_path):
                for file in files:
                    abs_path = os.path.join(root, file)
                    arc_path = os.path.relpath(abs_path, start=os.path.commonpath(directories))
                    zipf.write(abs_path, arc_path)


def copy_dir(src_dir, dst_dir):
    for root, dirs, files in os.walk(src_dir):
        # Compute destination path
        relative_path = os.path.relpath(root, src_dir)
        dst_path = os.path.join(dst_dir, relative_path)

        # Create destination directory if it doesn't exist
        os.makedirs(dst_path, exist_ok=True)

        # Copy all files in the current directory
        for file in files:
            src_file = os.path.join(root, file)
            dst_file = os.path.join(dst_path, file)

            shutil.copy(src_file, dst_file)
            os.chmod(dst_file, stat.S_IWRITE)


def copy_file(file, dest_folder):
    shutil.copy(file, dest_folder)


def package_dir(config_path, useDistribution):
    config_file = open(config_path)
    config_data = json.load(config_file)

    output_dir = config_data['output']
    os.makedirs(output_dir, exist_ok=True)

    for executable in config_data['executables']:
        copy_file(executable, output_dir)

    for dir in config_data['assets']:
        copy_dir(dir, output_dir + "/" + dir)

    for file in config_data['files']:
        copy_file(file, output_dir)

    if useDistribution:
        zip_directories(output_dir + "/data.bin", ["assets", "game", "shaders"])
        shutil.rmtree(output_dir + "/assets")
        shutil.rmtree(output_dir + "/game")
        shutil.rmtree(output_dir + "/shaders")


def main():
    parser = argparse.ArgumentParser(description='Packages the project to /package/')
    parser.add_argument('-c', '--config', help="JSON config for packaging", type=str, required=True)
    parser.add_argument('-dist', '--distribution', help="Sets the packaging ready for distribution",
                        action='store_true', required=True)

    args = parser.parse_args()
    package_dir(args.config, args.distribution)


if __name__ == "__main__":
    main()
