#include <lib/string.h>
#include <lib/mem.h>
#include <lib/utils.h>
#include <stdint.h>

/*
 * FAT32 short name generation
 * Based on the FAT32 specification.
 * 
 * "The Basis-Name Generation Algorithm", fatgen103 (29 - 31)
 */

#define MAX_BASENAME 8
#define MAX_EXTENSION 3
#define DIR_ENTRY_SIZE 11

#define ILLEGAL_CHARS "\"*+,./:;<=>?[\\]|"
#define ILLEGAL_CHARS_REPLACE '_'

static int _toupper(int c){
    if(c >= 'a' && c <= 'z'){
        c -= 32;
    }

    return c;
}

static char _convert_char(char c){
    const char illegal[] = "\"*+,./:;<=>?[\\]|";
    if(c < 0x20 || strchr(ILLEGAL_CHARS, c)){
        return ILLEGAL_CHARS_REPLACE;
    }

    return _toupper((unsigned char)c);
}

// Remove spaces on the right
static void _rtrim(char *str){
    int len = strlen(str);
    while(len > 0 && str[len - 1] == ' '){
        str[--len] = '\0';
    }
}

void _fat32_append_tilde(char *outname, int n){
    char base[MAX_BASENAME + 1] = {0};
    memcpy(base, outname, 8);
    base[MAX_BASENAME] = '\0';

    char suffix[6];
    suffix[0] = '~';
    itoa(n, suffix + 1, 10);

    int baseLen = strlen(base);
    int suffixLen = strlen(suffix);

    if(baseLen + suffixLen > MAX_BASENAME){
        baseLen = MAX_BASENAME - suffixLen;
    }

    base[baseLen] = '\0';
    strcat(base, suffix);

    memcpy(outname, base, 8);
}

void _fat32_generate_short_name(const char *name, char *out){
    char base[MAX_BASENAME + 1] = {0};
    char ext[MAX_EXTENSION + 1] = {0};
    const char *dot = strrchr(name, '.');

    if(dot && dot != name){
        for(int i = 0; i < MAX_EXTENSION && dot[1 + i] != '\0'; i++){
            ext[i] = _convert_char(dot[1 + i]);
        }
    }

    int i = 0, j = 0;
    for(i = 0; name[i] != '\0' && name + i != dot && j < MAX_BASENAME; i++){
        base[j++] = _convert_char(name[i]);
    }

    base[j] = '\0';
    _rtrim(base);

    memset(out, ' ', DIR_ENTRY_SIZE);
    memcpy(out, base, strlen(base));
    memcpy(out + 8, ext, strlen(ext));
}
