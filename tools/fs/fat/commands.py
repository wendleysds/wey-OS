from lib.fat.defs import FAT32, FAT32DirectoryEntry, FATFS
import lib.fat.attributes as attr

import os, sys

def cluster_to_lba(fat: FAT32, cluster: int) -> int:
    return fat.firstDataSector + ((cluster - 2) * fat.headers.boot.secPerClus)

def get_cluster_entry(entry: FAT32DirectoryEntry) -> int:
    return (entry.fstClusHI << 16) | entry.fstClusLO

def creat(fs: FATFS, path: str) -> int:
    # /some/path/file -> dirs = /some/path ; name = file
    tokens = path.rstrip('/').split('/')
    name = tokens[len(tokens)-1]
    dirs = path[:-len('/' + name)]

    entry = fs.walk('/' if dirs == '' else dirs)
    if not entry:
        return 1
    
    cluster = get_cluster_entry(entry)
    
    r = fs.create(cluster, name, attr.ARCHIVE)
    if r == -2:
        print("File exists")
        return 1
    
    if r == -1:
        print("FAT Full")
        return 1
    
    return 0

def unlink(fs: FATFS, path: str) -> int:
    tokens = path.rstrip('/').split('/')
    name = tokens[len(tokens)-1]
    dirs = path[:-len('/' + name)]

    if dirs == '':
        print("Invalid file")
        return 0

    entry = fs.walk('/' if dirs == '' else dirs)
    if not entry:
        print("No such file or directory")
        return 1
    
    cluster = get_cluster_entry(entry)
    target = fs.search(cluster, name)

    if not target:
        print("No such file or directory")
        return 1

    if not target.attr & attr.ARCHIVE:
        print("Is a directory")
        return 0
    
    if not (target.attr & attr.SYSTEM) == 0:
        print("Its a system file")
        return 0
    
    if fs.remove(cluster, name) != 0:
        print("No such file or directory")
        return 0

def mkdir(fs: FATFS, path: str) -> int:
    tokens = path.rstrip('/').split('/')
    name = tokens[len(tokens)-1]
    dirs = path[:-len('/' + name)]

    entry = fs.walk('/' if dirs == '' else dirs)
    if not entry:
        return 1
    
    cluster = get_cluster_entry(entry)
    
    r = fs.create(cluster, name, attr.DIRECTORY)
    if r == -2:
        print("File exists")
        return 1
    
    if r == -1:
        print("FAT Full")
        return 1

    return 0

def rmdir(fs: FATFS, path: str) -> int:
    tokens = path.rstrip('/').split('/')
    name = tokens[len(tokens)-1]
    dirs = path[:-len('/' + name)]

    entry = fs.walk('/' if dirs == '' else dirs)
    if not entry:
        print("No such file or directory")
        return 1
    
    if not entry.attr & attr.DIRECTORY:
        print("Not a directory")
        return 1
    
    cluster = get_cluster_entry(entry)
    target = fs.search(cluster, name)

    if not target:
        print("No such file or directory")
        return 1

    if len(fs.ls(get_cluster_entry(target))) - 2 != 0:
        print("Directory not empty")
        return 1
    
    res = fs.remove(cluster, name)
    if res != 0:
        print("No such file or directory")
        return 1

def ls(fs: FATFS, path: str) -> int:
    entry = fs.walk(path)
    if not entry:
        print("No such file or directory")
        return 1
    
    if not entry.attr & attr.DIRECTORY:
        print(path)
        return
    
    entries = fs.ls(get_cluster_entry(entry))

    for e in entries:
        name = e.name.decode().strip()
        if e.attr & attr.DIRECTORY:
            print(f"\033[1;34m{name}\033[0m/", end='')
        else:
            print(f"{name[:8].rstrip()}.{name[8:11].rstrip()}", end='')

        print("   ", end='')
    print('')

def read(fs: FATFS, args: list[str], path: str) -> int:
    fd = fs.open(path)
    if not fd:
        print("No such file or directory")
        return 1
    
    if fd.entry.fileSize == 0:
        print("File is empty")
        return 0
    
    if fd.entry.attr & attr.DIRECTORY:
        print("Is a directory")
        return 0
    
    seek = 0
    readSize = fd.entry.fileSize
    printbytes = False

    try:
        if '-s' in args:
            seek = int(args[args.index('-s') + 1])

        if seek < 0 or seek >= fd.entry.fileSize:
            print("Seek out of bounds")
            return 1
            
        if '-l' in args:
            readSize = int(args[args.index('-l') + 1])
            if readSize < 0 or readSize > fd.entry.fileSize - seek:
                print("Read size out of bounds")
                return 1
            
        if '-b' in args:
            printbytes = True

    except (ValueError, IndexError):
        print("Invalid arguments")
        return 1


    if readSize <= 0:
        return 0

    bufferSize = 512

    fs.seek(fd, seek)

    while readSize > 0:
        toRead = min(readSize, bufferSize)
        data = fs.read(fd, toRead)

        if not data:
            break
        
        if printbytes:
            print("".join(f"{b:02x} " for b in data))
        else:
            print(data.decode(errors='ignore'), end='')

        readSize -= len(data)

    # check if last print is a \n
    if not printbytes and data and data[-1] != 10 and data[-1] != 0:
        print()

def stat(fs: FATFS, path: str) -> int:
    entry = fs.walk(path)
    if not entry:
        print("No such file or directory")
        return 1

    cluster = get_cluster_entry(entry)
    lba = cluster_to_lba(fs.fat, cluster)
    print(entry)

    print("\nCluster      : ", cluster)
    print(f"data LBA     :  {lba} - {lba*512}:{hex(lba*512)}")
    print("Chain        :  ", end='')

    cluster_chain = []
    cur = cluster
    while 0x2 <= cur < 0x0FFFFFF8:
        cluster_chain.append(cur)
        next = fs.fat.next_cluster(cur)
        cur = next

    print(" -> ".join(map(str, cluster_chain)) + " -> [EOF]")
    return 0

def cp(fs: FATFS, args: list[str], path1: str, path2: str) -> int:
    is_external = '-ex' in args

    if is_external:
        if not os.path.isfile(path1):
            print("No such file or directory")
            return 1
    else:
        if not fs.walk(path1):
            print("No such file or directory")
            return 1
    
    # dest is a folder        ./ext/foo.txt -> /home or /home/ (search and replace or create)
    # dest is a existent file ./ext/foo.txt -> /home/bar.txt (rewrite/replace)

    # create the destination if it does not exist
    dest_exists = fs.walk(path2)
    if not dest_exists:
        dirs = path2.removesuffix('/' + os.path.basename(path2))
        dir_entry = fs.walk(dirs if dirs != '' else '/')

        fs.create(get_cluster_entry(dir_entry), os.path.basename(path2), attr.ARCHIVE)
    
    # create the file if it does not exist
    if dest_exists and dest_exists.attr & attr.DIRECTORY:
        target_exists = fs.search(get_cluster_entry(dest_exists), os.path.basename(path1))

        # create if not exists
        if not target_exists:
            fs.create(get_cluster_entry(dest_exists), os.path.basename(path1), attr.ARCHIVE)

        path2 = os.path.join(path2, os.path.basename(path1))

    if is_external:
        with open(path1, '+rb') as f:
            fd = fs.open(path2)

            if not fd:
                print("No such file or directory")
                return 1

            if fd.entry.fileSize > 0:
                fs.fat.free_chain(fd.firstCluster)
                fs.fat.table[fd.firstCluster] = 0x0FFFFFF8
                fd.entry.fileSize = 0

            data = f.read(fs.fat.clusterSize)
            while data:
                fs.write(fd, data, len(data))
                data = f.read(fs.fat.clusterSize)
    else:
        srcFd = fs.open(path1)
        dstFd = fs.open(path2)

        if not srcFd or not dstFd:
            print("No such file or directory")
            return 1
        
        if dstFd.entry.fileSize > 0:
            fs.fat.free_chain(dstFd.firstCluster)
            fs.fat.table[dstFd.firstCluster] = 0x0FFFFFF8
            dstFd.entry.fileSize = 0

        data = fs.read(srcFd, fs.fat.clusterSize)
        while data:
            fs.write(dstFd, data, len(data))
            data = fs.read(srcFd, fs.fat.clusterSize)

    return 0
