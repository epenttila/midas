#!/usr/bin/env python3

import sys
from optparse import OptionParser
import midas.app

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('-t', '--test', action='store_true', dest='test', default=False)
    (options, args) = parser.parse_args()
    sys.exit(midas.app.Application(args[0], options.test).run())
