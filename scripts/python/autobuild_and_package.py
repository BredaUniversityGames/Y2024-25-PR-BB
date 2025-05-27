import os
import subprocess
from package import package_dir
import http.client
import json
import argparse

def build_wsl(project_path, build_name):

    wsl_path = project_path.replace("C:\\", "/mnt/c/").replace("\\", "/")
    print("WSL Path:", wsl_path)

    # TODO: this does not reconfigure for the first time, so it will fail if the build directory does not exist
    # command = ["wsl", "cd", wsl_path, "&&", "cmake", "--preset", build_name, "&&", "cmake", "--build", "build/" + build_name]
    command = ["wsl", "cd", wsl_path, "&&", "cmake", "--build", "build/" + build_name]
    return subprocess.run(command).returncode

def package_project(project_path, build_name):

    if build_name == "WSL-RelWithDebInfo":
        package_dir("scripts/build_system/package_config/linux_dev_rel_deb.json")
    elif build_name == "WSL-Release":
        package_dir("scripts/build_system/package_config/linux_dev_rel.json")
    else:
        print("Unsupported build name for packaging:", build_name)
    return 0

HEADER = { "Content-Type": "application/json" }
SUCCESS_DATA = {"type":"build", "status":"success", "name":"Blightspire"}
API_BASE_URL = "localhost"
API_PATH_NOTIFICATION = "/post_event"
API_PORT = 32010

def autobuild_notify():
    json_body = json.dumps(SUCCESS_DATA)
    
    # Open connection
    conn = http.client.HTTPConnection(API_BASE_URL, API_PORT)

    try:
        # Send request
        conn.request("POST", API_PATH_NOTIFICATION, body=json_body, headers=HEADER)

        # Get and print response
        response = conn.getresponse()
        print("Status:", response.status)
        print("Response:", response.read().decode())

    except Exception as e:
        print("Error sending event to Steam client:", str(e))
    finally:
        conn.close()

    return

def main():

    parser = argparse.ArgumentParser(description='Packages the project to /package/')
    parser.add_argument('-b', '--build', help="Build type", type=str, required=True)
    args = parser.parse_args()

    # TODO: in future we can add WSL-Release as well
    err = build_wsl(os.getcwd(), args.build)

    if err != 0:
        print("Error building project in WSL")
        return err
    
    package_project(os.getcwd(), args.build)
    autobuild_notify()


    return 0

if __name__ == "__main__":
    main()