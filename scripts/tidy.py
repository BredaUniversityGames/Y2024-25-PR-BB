import sys
import os
import pathlib
import subprocess
import argparse

# List of extensions to match
EXTENSIONS = {".cpp", ".hpp", ".h"}


def glob_recursive_files(path, extensions):
    ret = []
    path_obj = pathlib.Path(path)
    for file_path in path_obj.rglob("*"):
        if file_path.is_file() and file_path.suffix in extensions:
            ret.append(file_path)
    return ret


def call_clang_tidy(base_path, extensions):
    args = ["clang-tidy", "--enable-check-profile", "-p=build/x64-Debug/compile_commands.json",
            "--config-file=.clang-tidy"]
    args += glob_recursive_files(base_path, extensions)

    return subprocess.call(args)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--paths', nargs='*')

    args = parser.parse_args()

    for p in args.paths:
        call_clang_tidy(p, EXTENSIONS)


if __name__ == "__main__":
    main()
