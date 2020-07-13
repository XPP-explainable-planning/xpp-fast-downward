#! /usr/bin/env python3

import os
import sys


if __name__ == "__main__":
    cd = sys.argv.pop(1)
    print("Change working directory to: " + cd)
    os.chdir(cd)
    from driver.main import main
    main()
