char** environ;

void libc_main(int argc, char** argv, char** envp){
    environ = envp;
}