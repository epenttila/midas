#!/usr/bin/env python3

import sys
import midas.app

if __name__ == '__main__':
    sys.exit(midas.app.Application(sys.argv[1]).run())
