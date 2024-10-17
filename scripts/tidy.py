import subprocess
import pathlib
import time
import argparse

# Define the directory to search for source files
SOURCE = 'engine/source'

# Define the file extensions that clang-tidy should check
EXTENSION = ['.c', '.cpp', '.cc', '.h', '.hpp']

# Define your clang-tidy command and options
COMMAND = 'clang-tidy'
PROFILE_ENABLE = "--enable-check-profile"
COMPILE_DATABASE = 'build/x64-Release'
OPTIONS = '--config-file=scripts/.clang-tidy'


# OPTIONS = '--checks=file'

def glob_recursive_files(path, extensions):
    ret = []
    path_obj = pathlib.Path(path)
    for file_path in path_obj.rglob("*"):
        if file_path.is_file() and file_path.suffix in extensions:
            ret.append(file_path)
    return ret


def run_clang_tidy_on_files(files, profile):
    if profile:
        command = [COMMAND, '-p=' + COMPILE_DATABASE, PROFILE_ENABLE, OPTIONS, '--allow-no-checks'] + files

        with open(profile, 'w') as file:
            try:
                subprocess.call(command, stderr=file)

            except subprocess.CalledProcessError as e:
                print(f"Error running clang-tidy: {e}")
    else:
        command = [COMMAND, '-p=' + COMPILE_DATABASE, OPTIONS] + files
        try:
            subprocess.call(command)

        except subprocess.CalledProcessError as e:
            print(f"Error running clang-tidy: {e}")


def main():
    parser = argparse.ArgumentParser(description='Tidy directories of source files')
    parser.add_argument('-d', '--directory', help="Directories to Tidy", type=str, nargs='*')
    parser.add_argument('-f', '--files', help="Files to Tidy", type=str, nargs='*')
    parser.add_argument('-p', '--profile', help='Specify output directory to check profile', type=str)

    args = parser.parse_args()

    start_time = time.time()

    files = []

    if args.directory:
        for directory in args.directory:
            files += glob_recursive_files(directory, EXTENSION)

    if args.files:
        files += args.files

    print(f"Processing {len(files)} files.")

    run_clang_tidy_on_files(files, args.profile)
    end_time = time.time()

    elapsed_time = end_time - start_time
    print(f"Clang-tidy took {elapsed_time:.4f} seconds in total")
    print(f"Average of {(elapsed_time / len(files)):.4f} per file")


if __name__ == "__main__":
    main()
