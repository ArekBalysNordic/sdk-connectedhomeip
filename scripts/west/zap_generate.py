# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import os
import sys

from textwrap import dedent
from pathlib import Path
import shutil

from west import log
from west.commands import CommandError, WestCommand

from zap_common import existing_file_path, existing_dir_path, find_zap, ZapInstaller, DEFAULT_MATTER_PATH


class ZapGenerate(WestCommand):

    def __init__(self):
        super().__init__(
            'zap-generate',  # gets stored as self.name
            'Generate Matter data model files with ZAP',  # self.help
            # self.description:
            dedent('''
            Generate Matter data model files with the use of ZAP Tool
            based on the .zap template file defined for your application.'''))
        self.matter_path = DEFAULT_MATTER_PATH

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         formatter_class=argparse.RawDescriptionHelpFormatter,
                                         description=self.description)
        parser.add_argument('-z', '--zap-file', type=existing_file_path,
                            help='Path to data model configuration file (*.zap)')
        parser.add_argument('-o', '--output', type=existing_dir_path,
                            help='Path where to store the generated files')
        parser.add_argument('-m', '--matter-path', type=existing_dir_path,
                            default=DEFAULT_MATTER_PATH, help='Path to Matter SDK')
        parser.add_argument('-x', '--extra_clusters', nargs='+',
                            help='List of extra clusters to include')
        parser.add_argument('-s', '--simple', action='store_true', help='Generate only a .matter file')
        parser.add_argument('-k', '--keep-previous', action='store_true', help='Keep previously generated files')
        return parser

    def build_command(self, zap_file_path, output_path, templates_path=None):
        if templates_path is None:
            # Generate the .matter file from the .zap file
            cmd = [sys.executable, self.zap_generate_path, zap_file_path, "-o", output_path]
        else:
            # Generate source files from the .zap file
            cmd = [sys.executable, self.zap_generate_path, zap_file_path, "-o", output_path, "-t", templates_path]
        return [str(x) for x in cmd]

    def codegen_command(self, zap_file_path, output_path):
        print(zap_file_path.name)
        cmd = [sys.executable, self.codegen_path, f"{zap_file_path.parent}/{zap_file_path.stem}.matter",
               "-g", "cpp-sdk", "--output-dir", output_path]
        return [str(x) for x in cmd]

    def do_run(self, args, unknown_args):
        self.zap_generate_path = args.matter_path / "scripts/tools/zap/generate.py"
        self.codegen_path = args.matter_path / "scripts/codegen.py"
        self.matter_path = args.matter_path

        if args.zap_file:
            zap_file_path = args.zap_file.absolute()
        else:
            zap_file_path = find_zap()

        if not zap_file_path:
            raise CommandError("No valid .zap file provided")

        if args.output:
            output_path = args.output.absolute()
        else:
            output_path = zap_file_path.parent / "zap-generated"

        app_templates_path = args.matter_path / "src/app/zap-templates/app-templates.json"

        zap_installer = ZapInstaller(args.matter_path)
        zap_installer.update_zap_if_needed()

        # make sure that the generate.py script uses the proper zap_cli binary (handled by west)
        os.environ["ZAP_INSTALL_PATH"] = str(zap_installer.get_zap_cli_path().parent.absolute())

        # Make sure that output directory exists
        output_path.mkdir(exist_ok=True)

        if not args.keep_previous:
            self.clear_generated_files(output_path)

        # Generate .matter file
        self.check_call(self.build_command(zap_file_path, output_path))

        if not args.simple:
            # Generate source files
            self.check_call(self.build_command(zap_file_path, output_path, app_templates_path))

        if args.extra_clusters:
            output_path = output_path
            output_path.mkdir(parents=True, exist_ok=True)
            clusters_dir = output_path / "clusters"
            clusters_dir.mkdir(parents=True, exist_ok=True)
            zap_generated_cluster_dir = zap_file_path.parent / "clusters"
            zzz_generated_dir = args.matter_path / "zzz_generated/app-common/clusters"
            templates_path = args.matter_path / "src/app/common/templates/templates.json"

            # Generate files for the new clusters
            self.check_call(self.build_command(zap_file_path, clusters_dir, templates_path))
            self.check_call(self.codegen_command(zap_file_path, clusters_dir))

            # Append path to the new clusters in zzz_generated/app-common/clusters/BUILD.gn
            for entry in args.extra_clusters:
                self.move_zap_generated_cluster_files(zap_generated_cluster_dir, clusters_dir, entry)
                self.append_cluster_to_build_gn(zzz_generated_dir / "BUILD.gn", clusters_dir / entry)

            # Remove unused clusters, and leave only the ones in args.extra_clusters
            # self.remove_unused_clusters(zap_generated_cluster_dir, clusters_dir, args.extra_clusters)

        log.inf(f"Done. Files generated in {output_path}")

    def clear_generated_files(self, path: Path):
        log.inf("Clearing previously generated files:")
        for file in path.iterdir():
            if file.is_file():
                with open(file, 'r') as f:
                    for line in f.readlines():
                        if "// THIS FILE IS GENERATED BY ZAP" in line:
                            log.inf(f"\tRemoving {file}")
                            file.unlink()
                            break

    def remove_unused_clusters(self, zap_generated_cluster_dir: Path, clusters_dir: Path, extra_clusters: list[str]):
        # Remove from output_path/clusters all directories except those in args.extra_clusters
        if clusters_dir.exists() and clusters_dir.is_dir():
            # Normalize the allowed directory names to strings
            allowed_dirs = set(str(name) for name in extra_clusters)
            for entry in clusters_dir.iterdir():
                if entry.is_dir() and entry.name not in allowed_dirs:
                    # Remove the directory and all its contents, even if not empty
                    try:
                        shutil.rmtree(entry)
                    except Exception:
                        pass
                elif entry.is_file():
                    try:
                        entry.unlink()
                    except Exception:
                        pass
            try:
                shutil.rmtree(zap_generated_cluster_dir)
            except Exception:
                pass

    def move_zap_generated_cluster_files(self, zap_generated_dir: Path, clusters_dir: Path, cluster_name: str):
        """
        Move the generated files for a specific cluster from zap_generated_dir to clusters_dir/cluster_name.
        """
        src_dir = zap_generated_dir / cluster_name
        dst_dir = clusters_dir / cluster_name

        if not src_dir.exists() or not src_dir.is_dir():
            log.wrn(f"Source directory {src_dir} does not exist or is not a directory.")
            return

        # Ensure the destination directory exists
        dst_dir.mkdir(parents=True, exist_ok=True)

        # Move all files from src_dir to dst_dir
        for item in src_dir.iterdir():
            target = dst_dir / item.name
            if item.is_dir():
                shutil.move(str(item), str(target))
            else:
                item.rename(target)

    def append_cluster_to_build_gn(self, build_gn_path: Path, cluster_path: str):
        """Append a new cluster entry to the BUILD.gn file's public_deps section."""
        if not build_gn_path.exists():
            log.wrn(f"BUILD.gn file not found at {build_gn_path}")
            return

        # Read the current BUILD.gn file
        with open(build_gn_path, 'r') as f:
            content = f.read()

        # Find the last cluster entry in public_deps to insert the new one
        # Look for the pattern before the closing bracket of public_deps
        cluster = Path(cluster_path).relative_to(self.matter_path, walk_up=True)
        cluster_entry = f'      "${{chip_root}}/{cluster}" + invoker.target,\n'

        # Find the position to insert the new cluster entry
        # Look for the last existing cluster entry and insert after it
        lines = content.splitlines()
        insert_index = -1

        # Check if the cluster entry is already present
        if any(cluster_entry.strip() == line.strip() for line in lines):
            log.inf(f"{cluster.name} cluster already present in BUILD.gn, skipping append.")
            return

        for i, line in enumerate(lines):
            if '" + invoker.target,' in line and 'zzz_generated/app-common/clusters/' in line:
                insert_index = i

        if insert_index != -1:
            # Insert the new cluster entry after the last existing cluster
            lines.insert(insert_index + 1, cluster_entry.rstrip())

            # Write the modified content back to the file
            with open(build_gn_path, 'w') as f:
                f.write('\n'.join(lines) + '\n')

            log.inf(f"Added {cluster.name} cluster to BUILD.gn")
        else:
            log.wrn(f"Could not find appropriate location to insert {cluster.name} in BUILD.gn")
