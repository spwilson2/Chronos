#!/usr/bin/python
#boot-sign takes as an argument the file name of a 512 byte file that is
#ready to be signed. This boot-sign should return a non zero value if
#any error is encountered.

import sys, os, struct

ENDIANESS = 'little'

MAP_ENDIANESS = { 'little': '<', 'big': '>'}

BOOT_LOCATION = 510

BOOT_MAGIC_NUM = struct.pack(MAP_ENDIANESS[ENDIANESS] + 'H', 0xAA55)

def main():
    if len(sys.argv) != 2:
        return 1
    filename = sys.argv[1]

    print('*************** Boot Sign Utility ***************')

    try:
        file_ =open(filename, "r+b")
        filesize = os.stat(filename).st_size

        # If we can fit in the boot sign, write it.
        if filesize < 510:
            file_.seek(BOOT_LOCATION, os.SEEK_SET)
            file_.write(BOOT_MAGIC_NUM)
            print('Writing boot sector.')

        # Check if it is signed, if it is we don't need to sign, if it's not
        # then the file is too big to sign.
        elif filesize == BOOT_LOCATION + len(BOOT_MAGIC_NUM):
            file_.seek(BOOT_LOCATION, os.SEEK_SET)

            if BOOT_MAGIC_NUM != file_.read(len(BOOT_MAGIC_NUM)):
                print('ERROR, Boot sector is too large!')
                return 1
            else:
                print('Boot sector is already signed.')
                return 0

        else:
            print('Error: Boot sector is too large')
            return 1

    except:
        print('Unable to open file!')
    else:
        close(file_)
        return 0

    return 1

if __name__ == '__main__':
    main()
