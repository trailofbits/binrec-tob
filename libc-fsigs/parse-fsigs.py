#!/usr/bin/env python3
import sys
import re
import fileinput

sizes = {
    'char': 1,
    'short': 2,
    'int': 4,
    'long': 4,
    'long int': 4,
    'long long': 8,
    'void': 0,
    'void*': 4,
    'size_t': 4,
    'uint8_t': 4,
    'uint16_t': 2,
    'uint32_t': 4,
    'uint64_t': 8,
    'float': 4,
    'double': 8,
    'unsigned': 4,

    # from './gettypes'
    'ACTION': 4,
    'ENTRY': 8,
    'bool_t': 4,
    'caddr_t': 4,
    'cap_user_data_t': 4,
    'cap_user_header_t': 4,
    'dev_t': 8,
    'div_t': 8,
    'enum auth_stat': 4,
    'enum clnt_stat': 4,
    'enum mcheck_status': 4,
    'enum xdr_op': 4,
    'error_t': 4,
    'gid_t': 4,
    'iconv_t': 4,
    'in_addr_t': 4,
    'intmax_t': 8,
    'jmp_buf': 156,
    'key_t': 4,
    'ldiv_t': 8,
    'lldiv_t': 16,
    'long double': 12,
    'long long int': 8,
    'mode_t': 4,
    'nl_catd': 4,
    'nl_item': 4,
    'off_t': 4,
    'pid_t': 4,
    'pthread_t': 4,
    'sa_family_t': 2,
    'sighandler_t': 4,
    'speed_t': 4,
    'ssize_t': 4,
    'struct in_addr': 4,
    'time_t': 4,
    'uid_t': 4,
    'uintmax_t': 8,
    'useconds_t': 4,
    'va_list': 4,
    'wchar_t': 4,
    'wint_t': 4,
    'xdrproc_t': 4,
    'clock_t': 4,
    'wctrans_t': 4,
    'wctype_t': 4,
    'sigjmp_buf': 156,
    'union sigval': 4,
    'clockid_t': 4,
    'idtype_t': 4,
    'id_t': 4,
    'intptr_t': 4,
    'off64_t': 8,
    'nfds_t': 4,
    'ptrdiff_t': 4,
    'struct mallinfo': 40,
    'imaxdiv_t': 16,
}


def argtype(s):
    if '*' in s or '[' in s:
        return 'void*'

    parts = s.split()[:-1]

    while len(parts) > 1 and parts[0] in ('const', 'unsigned'):
        parts = parts[1:]

    return ' '.join(parts)


def argsize(s):
    t = argtype(s)
    return sizes.get(t, '<error: %s>' % t)


if __name__ == '__main__':
    # print '<name> <isfloat> <retsize> <argsizes> for each function
    for line in fileinput.input():
        m = re.match(r'^((?:[^ ]+ +)+?\*?([^ (]+)) *\((.*)\);', line)
        if not m:
            print('error:', line, file=sys.stderr)
            continue

        ftypename, fname, args = m.groups()

        if args in ('', 'void'):
            args = []
        else:
            args = re.split(r', *', args)

            # filter out vararg functions
            if args[-1] == '...':
                continue

        isfloat = 'float' in ftypename or 'double' in ftypename
        print('%s %d %s%s' % (fname, int(isfloat), argsize(ftypename),
                              ''.join(' ' + str(argsize(x)) for x in args)))
