import sys
import os
import pathlib
import subprocess

# List of extensions to match
EXTENSIONS = {".cpp", ".hpp", ".h"}


def glob_recursive_files(path, extensions):
    ret = []
    path_obj = pathlib.Path(path)
    for file_path in path_obj.rglob("*"):
        if file_path.is_file() and file_path.suffix in extensions:
            ret.append(file_path)
    return ret


def call_clang_format(base_path, extensions):
    args = ["clang-format", "-i", "-style=file:.clang-format"]
    args += glob_recursive_files(base_path, extensions)

    return subprocess.call(args)


def main():
    for i in range(1, len(sys.argv)):
        call_clang_format(sys.argv[i], EXTENSIONS)
        break
    else:
        call_clang_format(os.getcwd(), EXTENSIONS)


if __name__ == "__main__":
    main()
