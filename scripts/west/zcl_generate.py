#!/usr/bin/env python
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import os
import json

from west.commands import WestCommand

class ZCLGenerate(WestCommand):

    def __init__(self):
        super().__init__(
            'zcl-generate',
            'Generate ZCL Matter Data Model file',
            'A tool for generating ZCL Matter Data Model file according to the base ZCL file and external clusters definitions.')

    def do_add_parser(self, parser_adder) -> argparse.ArgumentParser:
        parser = parser_adder.add_parser(self.name,
                                    help=self.help,
                                    formatter_class=argparse.RawDescriptionHelpFormatter,
                                    description=self.description)
        parser.add_argument("-b", "--base", type=str,
                        help="An absolute path to the base zcl.json file. If not provided the path will be set to ZEPHYR_BASE/../modules/lib/matter/src/app/zap-templates/zcl/zcl.json.")
        parser.add_argument("-m", "--matter", type=str,
                            help="An absolute path to the Matter directory. If not set the path with be set to the ZEPHYR_BASE/../modules/lib/matter")
        parser.add_argument("-o", "--output", type=str,
                            help="Output path to store the generated zcl .json file. If not provided the path will be set to the ./zcl.json.", default="./zcl.json")
        parser.add_argument('--append', action="store_true", default=False,
                            help="Append to the existing ZCL file instead of creating the new one. This means that the ZCL file located in the Matter directory will be edited and new clusters definitions will be appended.")
        parser.add_argument("new_clusters", nargs='+',
                            help="Paths to the XML files that contains the external cluster definitions")
        return parser

    def do_run(self, args, unknown_args) -> None:
        if not args.matter:
            if not "ZEPHYR_BASE" in os.environ:
                print("ERROR: ZEPHYR_BASE is not set in the system path")
                return
            MATTER_PATH = os.environ["ZEPHYR_BASE"] + \
                "/../modules/lib/matter"
        else:
            MATTER_PATH = args.matter

        if not args.base:
            if not "ZEPHYR_BASE" in os.environ:
                print("ERROR: ZEPHYR_BASE is not set in the system path")
                return
            BASE_PATH = MATTER_PATH + "/src/app/zap-templates/zcl/zcl.json"
        else:
            BASE_PATH = args.base

        file_to_write = args.output if not args.append else BASE_PATH
        directory_of_output_file = os.path.dirname(os.path.abspath(file_to_write))

        try:
            with open(BASE_PATH, "r") as zcl_base:
                zcl_json_base = json.load(zcl_base)
        except IOError:
            print(f"No such ZCL file: {BASE_PATH}")
            return

        if not args.append:
            # Calculate the relative path from provided output .json file to the Matter data model
            PATH_TO_DATA_MODEL = os.path.relpath(
                MATTER_PATH, directory_of_output_file) + "/src/app/zap-templates/zcl"
            roots_replaced = list()

            # Replace paths
            for i, path in enumerate(zcl_json_base.get("xmlRoot")):
                if i != 0:
                    roots_replaced.append(path.replace(
                        "./", PATH_TO_DATA_MODEL + "/"))
            zcl_json_base["xmlRoot"] = roots_replaced
            # Add path to the data model base directory
            zcl_json_base.get("xmlRoot").append("./")
            zcl_json_base.get("xmlRoot").append(
                PATH_TO_DATA_MODEL + "/data-model/")
            # Add path to manufacturers XML
            zcl_json_base["manufacturersXml"] = f"{PATH_TO_DATA_MODEL}/data-model/manufacturers.xml"

        for cluster in args.new_clusters:
            base = os.path.abspath(cluster)
            base_rel = os.path.dirname(os.path.relpath(base, directory_of_output_file))
            file = os.path.basename(cluster)
            if not base_rel + "/" in zcl_json_base.get("xmlRoot"):
                zcl_json_base.get("xmlRoot").append(base_rel + "/")
            if not file in zcl_json_base.get("xmlFile"):
                zcl_json_base.get("xmlFile").append(file)

        json_dump = json.dumps(zcl_json_base, indent=4)

        with open(file_to_write, "w+") as zcl_output:
            zcl_output.write(json_dump)
