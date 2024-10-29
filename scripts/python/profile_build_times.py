import argparse
import sys
import time
import subprocess


def check_ret(val):
    if val != 0:
        sys.exit()


def clean_files(build_path):
    args = ["cmake", "--build", build_path, "--target clean"]
    return subprocess.run(args).returncode


def configure_cmake(cmake_args):
    flags = cmake_args.split()
    return subprocess.run(["cmake"] + flags).returncode


def build_cmake(build_out, args):

    command = ["cmake", "--build", build_out]

    if args:
        command.append(args)

    return subprocess.run(command).returncode


def main():
    parser = argparse.ArgumentParser(description='Test and Profile build configurations')

    parser.add_argument(
        '-c', '--configs', type=str, required=True, nargs='*',
        help='Specify configuration arguments to test'
    )

    parser.add_argument('-b', '--build_dir', help="Build directory", type=str, required=True)
    parser.add_argument('-f', '--flags', help="Build flags", type=str)
    args = parser.parse_args()

    time_list = []

    for config in args.configs:
        check_ret(configure_cmake(config))
        check_ret(clean_files(args.build_dir))

        start_time = time.time()
        check_ret(build_cmake(args.build_dir, args.flags))
        elapsed_time = time.time() - start_time
        time_list.append(f"Build took {elapsed_time:.4f} seconds: Configuration {config}")

    print("##########################")
    print("")

    for result in time_list:
        print(result)

    print("")
    print("##########################")

if __name__ == "__main__":
    main()
