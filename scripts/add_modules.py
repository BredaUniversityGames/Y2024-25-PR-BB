import sys;
import subprocess;

THIRD_PARTY_ROOT = "external/"

def check_result(code):
    if code != 0:
        sys.exit(1)

def add_submodule(git_link):
    folder_name = git_link[git_link.rfind("/")+1:git_link.rfind(".git")]
    return subprocess.run(["git", "submodule", "add", git_link, folder_name]).returncode

def main():
    for arg in sys.argv[1:]:
        check_result(add_submodule(arg))

    return 0

if __name__ == "__main__":
    main()