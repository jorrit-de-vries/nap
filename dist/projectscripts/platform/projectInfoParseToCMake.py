#!/usr/bin/python

import argparse
import json
import os
import sys

from NAPShared import find_project

PROJECT_INFO_FILENAME = 'project.json'
PROJECT_INFO_CMAKE_CACHE_FILENAME = 'cached_project_json.cmake'

def update_project_info_to_cmake(project_name):
    project_path = find_project(project_name)
    if project_path is None:
        return False

    output_filename = os.path.join(project_path, PROJECT_INFO_CMAKE_CACHE_FILENAME)

    # If any existing output file exists remove it.  This ensure that CMake will fail if our JSON parsing etc fails.
    if os.path.exists(output_filename):
        os.remove(output_filename)

    with open(os.path.join(project_path, PROJECT_INFO_FILENAME)) as json_file:
        json_dict = json.load(json_file)
        if not 'modules' in json_dict:
            print("Missing element 'modules' in %s" % PROJECT_INFO_FILENAME)
            return False

        if not type(json_dict['modules']) is list:
            print("Element 'modules' in %s is not an array" % PROJECT_INFO_FILENAME)
            return False

        nap_modules = ' '.join(json_dict['modules'])
        print("Built modules: %s" % nap_modules)

    with open(output_filename, 'w') as out_file:
        # out_file.write("project(%s)\n" % project_name)
        out_file.write("set(NAP_MODULES %s)\n" % nap_modules)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('PROJECT_NAME')
    args = parser.parse_args()

    if not update_project_info_to_cmake(args.PROJECT_NAME):
        sys.exit(1)
