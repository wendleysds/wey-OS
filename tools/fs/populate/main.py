from fat import FAT
from typing import List
import sys

fat: FAT

def help():
    print("Usage: <img> <option> <dest> <files...>")
    print("\nOptions:")
    print("\th: display this message;")
    print("\tc: copy <files...> in <dest> to <img> file system;")
    print("\tg: generete <files...> in <dest> to <img> file system;")
    print("\ti: start a interactive shell with simple commands(mkdir, rmdir, rm, create...) with <img>.")

def main(argc: int, argv: List[str]):
    if argc == 1 or argc < 3:
        if argc == 2 and argv[1] == '-h':
            help()
            exit(0)

        print(f"Try: py {argv[0]} -h")
        exit(1)

main(len(sys.argv), sys.argv)