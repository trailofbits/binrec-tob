#define GET(ty) printf("    '" #ty "': %u,\n", sizeof (ty))

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <argz.h>
#include <iconv.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <mcheck.h>
#include <wchar.h>
#include <nl_types.h>
#include <termios.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <sys/capability.h>
#include <sys/times.h>
#include <wctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/prctl.h>
#include <malloc.h>
#include <inttypes.h>

typedef void (*sighandler_t)(int);

int main(void) {
    GET(ACTION);
    GET(ENTRY);
    GET(bool_t);
    GET(caddr_t);
    GET(cap_user_data_t);
    GET(cap_user_header_t);
    GET(dev_t);
    GET(div_t);
    GET(enum auth_stat);
    GET(enum clnt_stat);
    GET(enum mcheck_status);
    GET(enum xdr_op);
    GET(error_t);
    GET(gid_t);
    GET(iconv_t);
    GET(in_addr_t);
    GET(intmax_t);
    GET(jmp_buf);
    GET(key_t);
    GET(ldiv_t);
    GET(lldiv_t);
    GET(long double);
    GET(long long int);
    GET(mode_t);
    GET(nl_catd);
    GET(nl_item);
    GET(off_t);
    GET(pid_t);
    GET(pthread_t);
    GET(sa_family_t);
    GET(sighandler_t);
    GET(speed_t);
    GET(ssize_t);
    GET(struct in_addr);
    GET(time_t);
    GET(uid_t);
    GET(uintmax_t);
    GET(useconds_t);
    GET(va_list);
    GET(wchar_t);
    GET(wint_t);
    GET(xdrproc_t);
    GET(clock_t);
    GET(wctrans_t);
    GET(wctype_t);
    GET(sigjmp_buf);
    GET(union sigval);
    GET(clockid_t);
    GET(idtype_t);
    GET(id_t);
    GET(intptr_t);
    GET(off64_t);
    GET(nfds_t);
    GET(ptrdiff_t);
    GET(struct mallinfo);
    GET(imaxdiv_t);

    return 0;
}
