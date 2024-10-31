import argparse
import subprocess
import requests
import json
import sys

STEAMDECK_NOTIFY_URL = "http://127.0.0.1:32010/post_event"
NOTIFY_HEADER = {"Content-Type": "application/json"}


def check_ret(result):
    if result != 0:
        sys.exit(-1)
    return


# def run_wsl_subprocess(args):
#     return subprocess.run(["wsl"] + args).returncode
#
#
# def configure_and_build(config):
#     cmake_configure = ["cmake", "--preset", config]
#     cmake_build = ["cmake", "--build", "--preset", config]
#
#     check_ret(run_wsl_subprocess(cmake_configure))
#     check_ret(run_wsl_subprocess(cmake_build))
#     return


def package(config):
    out_path = "build/" + config
    python_args = ["python", "scripts/package.py", "-o", "package", "-e", out_path + "/Game", "-a", "assets",
                   "shaders/bin"]
    check_ret(subprocess.run(python_args).returncode)
    return


def notify_steam_devkit(name):
    build_success_notify = {"type": "build", "status": "success", "name": name}
    json_data = json.dumps(build_success_notify)

    try:
        # Send the POST request
        response = requests.post(STEAMDECK_NOTIFY_URL, data=json_data, headers=NOTIFY_HEADER)

        # Check for HTTP errors
        if response.status_code == 500:
            print(
                "Steam Deck upload error: Cannot connect to Steam Deck. Is the Steam Deck powered on and in Game Mode?")
        elif response.status_code != 200:
            print(f"Steam Deck upload error: {response.status_code} {response.reason}")
        else:
            print("Uploaded to Steam Deck")

    except requests.ConnectionError:
        print("Steam Deck upload error: Cannot connect to Steam Deck. Is SteamOS Devkit Client running?")

    return


def main():
    parser = argparse.ArgumentParser(
        description='Build, package and deploy a CMakePreset to a connected Steam Deck')

    # parser.add_argument('-p', '--preset', help="Compilation preset", type=str, required=True)
    parser.add_argument('-n', '--name', help="Name in Steam Deck client", type=str, required=True)

    args = parser.parse_args()

    # package(args.preset)
    notify_steam_devkit(args.name)


if __name__ == "__main__":
    main()
