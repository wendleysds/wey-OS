#!/usr/bin/env python3

import lib.fat.attributes as attr
from lib.fat.defs import FATFS

import commands as cmd
import sys, os

def read_config() -> dict:
    path = os.path.abspath(__file__)
    dir = os.path.dirname(path)
    config_path = os.path.join(dir, ".conf")
    
    config = {}
    with open(config_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" in line:
                key, value = line.split("=", 1)
                config[key.strip()] = value.strip()

    return config

def help():
    print(f"Usage: {sys.argv[0]} <option> <?args...> <path1> <path2>")
    print("\nDisclaimer: All paths must be absolute, starting with '/' or './'(local)")

    print("\tinit : initialize the FAT FS and essential folders and files")

    print("\tcreat: create a file in <path1>")
    print("\tunlink: remove a file in <path1>")

    print("\tmkdir: create a directory in <path1>")
    print("\trmdir: remove a directory in <path1>")

    print("\tls   : list directory contents in <path1>")

    print("\tread : read the file content in <path1>")
    print("\t\t-s: seek into the file")
    print("\t\t-l: the amount of bytes to print")
    print("\t\t-b: read the content in hex")

    print("\tstat : show file information like cluster chain, size, name, etc...")

    print("\tcp   : copy the file from <path1>(src) to <path2>(dst)")
    print("\t\t-ex: copy a external file in <path1>(local) to <path2>(img fs)")

def main():
    if len(sys.argv) < 2:
        print(f"try {sys.argv[0]} --help")
        return
    
    if sys.argv[1].replace("-","").lower() in ["h", "help"]:
        help()
        return

    conf = read_config()

    # folders
    script = os.path.abspath(__file__)
    home = os.path.dirname(script)
    bins = os.path.abspath(os.path.join(home, conf["BINS"]))

    # img
    img = os.path.abspath(os.path.join(home, conf["IMG"]))

    op = None
    args = []
    path1 = None
    path2 = None

    # Skip the script name
    argv = sys.argv[1:]

    if argv:
        op = argv[0]
        rest = argv[1:]
    else:
        rest = []

    # Extract paths (arguments containing '/')
    paths = [arg for arg in rest if '/' in arg]
    if len(paths) > 0:
        path1 = paths[0]
    if len(paths) > 1:
        path2 = paths[1]

    # Remaining args (not paths)
    args = [arg.lower() for arg in rest if '/' or './' not in arg]

    if op == "init":
        startCluster = conf["START_CLUSTER"]
        createFSInfo = conf["CREATE_FSINFO"]

        FATFS.format_file(None, img, createFSInfo == "True")
        fs = FATFS(img, int(startCluster))

        fs.create(2, "boot", attr.DIRECTORY | attr.SYSTEM | attr.READY_ONLY)
        fs.create(2, "home", attr.DIRECTORY | attr.SYSTEM | attr.READY_ONLY)
        fs.create(2, "bin", attr.DIRECTORY | attr.SYSTEM | attr.READY_ONLY)

        bootEntry = fs.search(2, 'boot')
        bootCluster = (bootEntry.fstClusHI << 16) | bootEntry.fstClusLO

        fs.create(bootCluster, 'init.bin', attr.ARCHIVE | attr.SYSTEM | attr.READY_ONLY)
        fs.create(bootCluster, 'kernel.bin', attr.ARCHIVE | attr.SYSTEM | attr.READY_ONLY)

        cmd.cp(fs, ['-ex'], os.path.join(bins, "init.bin"), '/boot/init.bin')
        cmd.cp(fs, ['-ex'], os.path.join(bins, "kernel.bin"), '/boot/kernel.bin')
        return 0
    
    fs = FATFS(img)
    
    operations = {
        "creat"  : lambda: cmd.creat(fs, path1),
        "unlink" : lambda: cmd.unlink(fs, path1),
        "mkdir"  : lambda: cmd.mkdir(fs, path1),
        "rmdir"  : lambda: cmd.rmdir(fs, path1),
        "ls"     : lambda: cmd.ls(fs, path1),
        "read"   : lambda: cmd.read(fs, args, path1),
        "stat"   : lambda: cmd.stat(fs, path1),
        "cp"     : lambda: cmd.cp(fs, args, path1, path2),
    }

    res: int = 0

    if op in operations:
        res = operations[op]()
    else:
        print(f"Invalid option! try {sys.argv[0]} --help")

    return res

if __name__ == "__main__":
    exit(main())