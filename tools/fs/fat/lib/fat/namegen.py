def fat_name_append_tilde(shortName: str) -> str:
    pass

def fat_name_generate_short(name: str) ->str:
    pass

def fat_format_name(filename: str) -> str:
    out = [' '] * 11

    if not filename or len(filename) == 0:
        return '\0'
    
    dot = filename.rfind('.')

    if len(filename) > 11:
        out[:8] = list(filename[:8])    
        if dot != -1 and dot + 1 < len(filename):
            ext = filename[dot + 1:]
            if len(ext) > 3:
                ext = ext[-3:]
            out[8:11] = list(ext.ljust(3))
        else:
            out[8:11] = list('   ')

        out[6] = '~'
        out[7] = '1'

        return ''.join(out).upper()
    
    name_length = 0
    ext_length = 0

    p = 0
    while p < len(filename) and (dot == -1 or p < dot) and name_length < 8:
        out[name_length] = filename[p]
        name_length += 1
        p += 1

    if dot != -1 and dot + 1 < len(filename):
        ext = filename[dot + 1:]
        while ext_length < len(ext) and ext_length < 3:
            out[8 + ext_length] = ext[ext_length]
            ext_length += 1

    return ''.join(out).upper()