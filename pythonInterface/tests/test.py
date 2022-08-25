# Copyright (c) 2022 Pascal Post
# This code is licensed under MIT license (see LICENSE.txt for details)

import cgns_tools_pySDK as m

# def test_main():
# assert m.__version__ == "0.0.1"
# assert m.add(1, 2) == 3
# assert m.subtract(1, 2) == -1

file = m.cgnsFile(
    "/home/pascal/workspace/cgns_struct2unstruct/test.cgns")
baseNames = file.baseInformation()

print(baseNames)
