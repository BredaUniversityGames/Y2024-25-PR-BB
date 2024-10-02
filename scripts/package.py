import shutil
import sys
import os
import argparse

def copy_dir(folder, dest):
    shutil.copytree(folder, dest, dirs_exist_ok=True)

def copy_file(file, dest_folder):
    shutil.copy(file, dest_folder)

def main():

    parser = argparse.ArgumentParser(description='Packages the project to /package/')
    parser.add_argument('-o', '--output', help="Output directory", type=str, required=True)
    parser.add_argument('-e','--executables', help='Files to copy over', type=str, nargs='*')
    parser.add_argument('-a','--assets', help='Asset folders to copy over', type=str, nargs='*')

    args = parser.parse_args()
    output_dir = str(args.output)

    os.makedirs(args.output, exist_ok=True)

    if args.assets:
        for dir in args.assets:
            copy_dir(dir, output_dir + "/" + dir)

    if args.executables:
        for file in args.executables:
            copy_file(file, output_dir)


if __name__ == "__main__":
    main()