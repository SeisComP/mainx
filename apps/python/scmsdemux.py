#!/usr/bin/env seiscomp-python

############################################################################
# Copyright (C) gempa GmbH                                                 #
# All rights reserved.                                                     #
# Contact: gempa GmbH (seiscomp-dev@gempa.de)                              #
#                                                                          #
# GNU Affero General Public License Usage                                  #
# This file may be used under the terms of the GNU Affero                  #
# Public License version 3.0 as published by the Free Software Foundation  #
# and appearing in the file LICENSE included in the packaging of this      #
# file. Please review the following information to ensure the GNU Affero   #
# Public License version 3.0 requirements will be met:                     #
# https://www.gnu.org/licenses/agpl-3.0.html.                              #
#                                                                          #
# Other Usage                                                              #
# Alternatively, this file may be used in accordance with the terms and    #
# conditions contained in a signed written agreement between you and       #
# gempa GmbH.                                                              #
############################################################################

import os
import sys

from getopt import gnu_getopt, GetoptError
from seiscomp import mseedlite as mseed


def usage():
    print(
        f"""Usage:
  {os.path.basename(__file__)} source

Demultiplex all miniSEED records found in the given source by stream code writing them
into separate new files. The source can be files or stdin. One file per stream is
generated. File names are derived from stream codes and the begin time of the records.

Verbosity:
  -h, --help            Display this help message.
  -v, --verbose         Verbose mode.

Examples:
Demultiplex the miniSEED records contained in data.mseed and additionally print the
names of created files to stderr
  {os.path.basename(__file__)} -v data.mseed

Demultiplex the miniSEED records received from stdin
  scmssort -u -E data.mseed | {os.path.basename(__file__)} -
"""
    )


def main():
    try:
        opts, args = gnu_getopt(
            sys.argv[1:],
            "hv",
            [
                "help",
                "verbose",
            ],
        )
    except GetoptError:
        print(
            f"{os.path.basename(__file__)}: Unknown option",
            file=sys.stderr,
        )
        usage()
        return False

    verbosity = False
    for flag, arg in opts:
        if flag in ("-h", "--help"):
            usage()
            return True

        if flag in ("-v", "--verbose"):
            verbosity = True

    inFile = sys.stdin.buffer
    try:
        if len(args[0]) > 0:
            openFiles = {}
    except Exception:
        print(
            f"{os.path.basename(__file__)}: Missing source",
            file=sys.stderr,
        )
        usage()
        sys.exit(1)

    if len(args) == 1:
        if args[0] != "-":
            try:
                inFile = open(args[0], "rb")
            except IOError as e:
                print(
                    f"Could not open input file '{args[0]}' for reading: {e}",
                    file=sys.stderr,
                )
                return False
        else:
            print(
                "Waiting for miniSEED records on stdin. Use Ctrl + C to interrupt.",
                file=sys.stderr,
            )
    elif len(args) != 0:
        usage()
        sys.exit(1)

    try:
        for rec in mseed.Input(inFile):
            oName = "%s.%s.%s.%s" % (rec.sta, rec.net, rec.loc, rec.cha)

            if oName not in openFiles:
                postfix = ".D.%04d.%03d.%02d%02d" % (
                    rec.begin_time.year,
                    rec.begin_time.timetuple()[7],
                    rec.begin_time.hour,
                    rec.begin_time.minute,
                )

                openFiles[oName] = open(oName + postfix, "ab")

            oFile = openFiles[oName]
            oFile.write(rec.header + rec.data)

        if verbosity:
            print("Generated output files:", file=sys.stderr)

        for oName in openFiles:
            if verbosity:
                print(f"  {oName}", file=sys.stderr)

            openFiles[oName].close()

    except KeyboardInterrupt:
        return True

    return True


if __name__ == "__main__":
    sys.exit(main())
