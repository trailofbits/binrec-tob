#include "binrec/rt/Print.h"
#include "binrec/rt/Syscall.h"
#include <array>
#include <cstdarg>

namespace {
template <typename It, typename F> void printRange(It begin, It end, F out) {
    for (; begin != end; ++begin) {
        out(*begin);
    }
}

template <typename F> void printInt(long long val, F out) {
    std::array<char, 20> buf;

    bool negative = val < 0;
    if (negative) {
        val = -val;
    }

    auto pos = buf.rbegin();
    do {
        *pos = val % 10 + '0';
        val /= 10;
        ++pos;
    } while (val > 0);

    if (negative) {
        *pos = '-';
        ++pos;
    }

    printRange(pos.base(), buf.end(), out);
}

template <typename F> void printUnsigned(unsigned long long val, F out) {
    std::array<char, 20> buf;

    auto pos = buf.rbegin();
    do {
        *pos = val % 10 + '0';
        val /= 10;
        ++pos;
    } while (val > 0);

    printRange(pos.base(), buf.end(), out);
}

template <typename F> void printHex(unsigned long long val, F out) {
    std::array<char, 18> buf;

    auto pos = buf.rbegin();
    do {
        int digit = val % 16;
        if (digit >= 10) {
            *pos = digit - 10 + 'A';
        } else {
            *pos = digit + '0';
        }
        val /= 16;
        ++pos;
    } while (val > 0);

    *pos = 'x';
    ++pos;
    *pos = '0';
    ++pos;

    printRange(pos.base(), buf.end(), out);
}

template <typename F> void printfImpl(F out, const char *format, va_list va) {
    bool inArg = false;

    for (char c = *format; c != 0; ++format, c = *format) {
        if (inArg) {
            switch (c) {
            case '%': {
                out('%');
                break;
            }
            case 'c': {
                char a = va_arg(va, int);
                out(a);
                break;
            }
            case 'd':
            case 'i': {
                int d = va_arg(va, int);
                printInt(d, out);
                break;
            }
            case 'u': {
                unsigned u = va_arg(va, unsigned);
                printUnsigned(u, out);
                break;
            }
            case 'x':
            case 'X': {
                unsigned x = va_arg(va, unsigned);
                printHex(x, out);
                break;
            }
            case 's': {
                const char *s = va_arg(va, const char *);
                for (char x = *s; x != 0; ++s, x = *s) {
                    out(x);
                }
                break;
            }
            case 'p': {
                unsigned long x = va_arg(va, unsigned long);
                printHex(x, out);
                break;
            }
            default:
                break;
            }
            inArg = false;
        } else if (c == '%') {
            inArg = true;
        } else {
            out(c);
        }
    }
}

int fdprintfImpl(int fd, const char *format, va_list va) {
    int written = 0;

    std::array<char, 128> buffer;
    auto pos = buffer.begin();
    auto end = buffer.end();
    auto outF = [&buffer, &pos, end, &written, fd](char c) {
        *pos = c;
        ++pos;
        if (pos == end) {
            written += binrec_rt_write(fd, buffer.data(), buffer.size());
            pos = buffer.begin();
        }
    };

    printfImpl(outF, format, va);

    int unwritten = pos - buffer.begin();
    written += binrec_rt_write(fd, buffer.data(), unwritten);

    return written;
}

} // namespace

int binrec_rt_printf(const char *format, ...) {
    va_list va;
    va_start(va, format);
    int written = fdprintfImpl(STDOUT_FILENO, format, va);
    va_end(va);
    return written;
}

int binrec_rt_fdprintf(int fd, const char *format, ...) {
    va_list va;
    va_start(va, format);
    int written = fdprintfImpl(fd, format, va);
    va_end(va);
    return written;
}

int binrec_rt_sprintf(char *buffer, size_t size, const char *format, ...) {
    char *pos = buffer;
    auto end = buffer + size - 1;
    auto outF = [&pos, end](char c) {
        if (pos != end) {
            *pos = c;
            ++pos;
        }
    };

    va_list va;
    va_start(va, format);
    printfImpl(outF, format, va);
    va_end(va);

    int unwritten = pos - buffer;
    int written = binrec_rt_write(STDOUT_FILENO, buffer, unwritten);

    return written;
}
