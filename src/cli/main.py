#!/usr/bin/env python3

import sys
from argparse import ArgumentParser
import midas.app

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-t', '--test', action='store_true', dest='test', default=False)
    parser.add_argument('-l', '--loglevel')
    parser.add_argument('file', metavar='FILE')
    args = parser.parse_args()
    sys.exit(midas.app.Application(args.file, args.test, args.loglevel).run())
