import subprocess
from unittest.mock import patch, MagicMock, call

import pytest

from binrec import _gdb_sigs as sigs

SETSID_MANPAGE = """.SH NAME
setsid - creates a session and sets the process group ID
.SH SYNOPSIS
.ad l
.B #include <unistd.h>
.sp
.B pid_t setsid(void);
.br
.ad b"""

FCNTL_MANPAGE = """.SH NAME
fcntl - manipulate file descriptor
.SH SYNOPSIS
.nf
.B #include <unistd.h>
.B #include <fcntl.h>
.sp
.BI "int fcntl(int " fd ", int " cmd ", ... /* " arg " */ );"
.fi
.SH DESCRIPTION
"""

SENDFILE_MANPAGE = """.SH NAME
sendfile - transfer data between file descriptors
.SH SYNOPSIS
.B #include <sys/sendfile.h>
.sp
.BI "ssize_t sendfile(int" " out_fd" ", int" " in_fd" ", off_t *" \\
                      offset ", size_t" " count" );
.\\" The below is too ugly. Comments about glibc versions belong
.\\" in the notes, not in the header.
"""

PIPE_MANPAGE = """.SH NAME
pipe, pipe2 - create pipe
.SH SYNOPSIS
.nf
.B #include <unistd.h>
.sp
.BI "int pipe(int " pipefd "[2]);"
.sp
.BR "#define _GNU_SOURCE" "             /* See feature_test_macros(7) */"
.BR "#include <fcntl.h>" "              /* Obtain O_* constant definitions */
.B #include <unistd.h>
.sp
.BI "int pipe2(int " pipefd "[2], int " flags );
"""

SIGANDSET_MANPAGE = """.nf
.BI "int sigisemptyset(const sigset_t *" set );
.BI "int sigorset(sigset_t *" dest ", const sigset_t *" left ,
.BI "              const sigset_t *" right );
.BI "int sigandset(sigset_t *" dest ", const sigset_t *" left ,
.br
.BI "              const sigset_t *" right );
.fi
"""

MOUNT_MANPAGE = """.SH NAME
mount - mount filesystem
.SH SYNOPSIS
.nf
.B "#include <sys/mount.h>"
.sp
.BI "int mount(const char *" source ", const char *" target ,
.BI "          const char *" filesystemtype ", unsigned long " mountflags ,
.BI "          const void *" data );
.fi
.SH DESCRIPTION
"""

INVALID_CONTINUATION_MANPAGE = """.SH NAME
mount - mount filesystem
.SH SYNOPSIS
.nf
.B "#include <sys/mount.h>"
.sp
.BI "int mount(const char *" source ", const char *" target ,
.BI "          const char *" filesystemtype ", unsigned long " mountflags ,
"          const void *" data );
.fi
.SH DESCRIPTION
"""


class TestGdbSigs:

    def test_clean_type_name(self):
        assert sigs._clean_type_name(" int  ") == "int"
        assert sigs._clean_type_name(" int *  ") == "void*"

    def test_split_function_signature(self):
        assert sigs._split_function_signature("int ( char , int* )") == ("int", ["char", "int*"])
        assert sigs._split_function_signature("void(void)") == ("void", [])
        assert sigs._split_function_signature("int rmdir(char *path)") == ("int rmdir", ["char *path"])
        assert sigs._split_function_signature("void (*)(int, char)(void (*)(int, char), int)") == ("void (*)(int, char)", ["void (*)(int, char)", "int"])

    def test_parse_gdb_function_signature(self):
        assert sigs._parse_gdb_function_signature("type = void(int, char*)") == ["void", "int", "void*"]
        assert sigs._parse_gdb_function_signature("type = <unknown type>") is None

    def test_parse_manpage_function_signature(self):
        assert sigs._parse_manpage_function_signature("int pipe(int pipefd[2])") == ["int", "void*"]
        assert sigs._parse_manpage_function_signature("int * do_stuff ( char  x , int )") == ["void*", "char", "int"]

    def test_extract_manpage_function_definition(self):
        assert sigs._extract_manpage_function_definition("setsid", SETSID_MANPAGE) == [
            "pid_t setsid(void);"
        ]

    def test_extract_manpage_function_definition_comment(self):
        assert sigs._extract_manpage_function_definition("fcntl", FCNTL_MANPAGE) == [
            "int fcntl(int  fd , int  cmd , ... /*  arg  */ );"
        ]

    def test_extract_manpage_function_definition_escape_linebreak(self):
        assert sigs._extract_manpage_function_definition("sendfile", SENDFILE_MANPAGE) == [
            "ssize_t sendfile(int  out_fd , int  in_fd , off_t *",
            "offset , size_t  count );"
        ]

    def test_extract_manpage_function_definition_array_arg(self):
        assert sigs._extract_manpage_function_definition("pipe", PIPE_MANPAGE) == [
            "int pipe(int  pipefd [2]);"
        ]

    def test_extract_manpage_function_definition_multiline_br(self):
        assert sigs._extract_manpage_function_definition("sigandset", SIGANDSET_MANPAGE) == [
            "int sigandset(sigset_t * dest , const sigset_t * left ,",
            "const sigset_t * right );"
        ]

    def test_extract_manpage_function_definition_multiline(self):
        assert sigs._extract_manpage_function_definition("mount", MOUNT_MANPAGE) == [
            "int mount(const char * source , const char * target ,",
            "const char * filesystemtype , unsigned long  mountflags ,",
            "const void * data );"
        ]

    def test_extract_manpage_function_definition_invalid(self):
        assert sigs._extract_manpage_function_definition("mount", INVALID_CONTINUATION_MANPAGE) is None

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1", "$1 = 8"]
        mock_parse_gdb.side_effect = [["type1"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 3
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("p sizeof(type1)", to_string=True),
        ], any_order=True)
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 0, 8)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_manpage_fallback(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1", "func2"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1", "sig2", "$1 = 4", "$2 = 8", "$3 = 12"]
        mock_parse_gdb.side_effect = [["type1", "type2"], None]
        mock_get_manpage.return_value = ["type1", "type3"]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 6
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("whatis func2", to_string=True),
            call("p sizeof(type1)", to_string=True),
            call("p sizeof(type2)", to_string=True),
            call("p sizeof(type3)", to_string=True)
        ], any_order=True)
        assert mock_parse_gdb.call_args_list == [
            call("sig1"),
            call("sig2")
        ]
        mock_get_manpage.assert_called_once_with("func2")

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_void(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1"]
        mock_parse_gdb.side_effect = [["void"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 2
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
        ])
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 0, 0)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_unk_size(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1", "error"]
        mock_parse_gdb.side_effect = [["type1"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 3
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("p sizeof(type1)", to_string=True),
        ], any_order=True)
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 0, 4)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_error_size(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.error = Exception
        mock_gdb.execute.side_effect = [None, "sig1", Exception("error")]
        mock_parse_gdb.side_effect = [["type1"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 3
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("p sizeof(type1)", to_string=True),
        ], any_order=True)
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 0, 4)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_float(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1", "$1 = 10"]
        mock_parse_gdb.side_effect = [["long double"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 3
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("p sizeof(long double)", to_string=True)
        ])
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 1, 10)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_i16(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1", "$1 = 2"]
        mock_parse_gdb.side_effect = [["short"], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 3
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
            call("p sizeof(short)", to_string=True)
        ])
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_called_once_with("SIG:func1", 0, 4)

    @patch.object(sigs, "_parse_gdb_function_signature")
    @patch.object(sigs, "_get_manpage_function_signature")
    @patch.object(sigs, "print")
    @patch.object(sigs, "log")
    @patch.object(sigs.sys, "stdin")
    def test_run_internal_vargs(self, mock_stdin, mock_log, mock_print, mock_get_manpage, mock_parse_gdb):
        mock_stdin.readline.return_value = "libc.so"
        mock_stdin.__iter__ = MagicMock(return_value=iter(["func1"]))
        mock_gdb = MagicMock()
        mock_gdb.execute.side_effect = [None, "sig1"]
        mock_parse_gdb.side_effect = [["..."], None]

        with patch.dict(sigs.sys.modules, {"gdb": mock_gdb}):
            sigs._run_internal()

        assert mock_gdb.execute.call_count == 2
        mock_gdb.execute.assert_has_calls([
            call("file libc.so"),
            call("whatis func1", to_string=True),
        ])
        assert mock_parse_gdb.call_args_list == [
            call("sig1")
        ]
        mock_get_manpage.assert_not_called()
        mock_print.assert_not_called()

    @patch.object(sigs, "GUESTFS_ROOT", new=None)
    def test_run_internal_no_guestfs(self):
        with patch.dict(sigs.sys.modules, {"gdb": MagicMock()}):
            with pytest.raises(KeyError):
                sigs._run_internal()

    @patch.object(sigs.os.path, "isfile")
    @patch.object(sigs.subprocess, "check_output")
    @patch.object(sigs, "_extract_manpage_function_definition")
    @patch.object(sigs, "_parse_manpage_function_signature")
    def test_get_manpage_function_signature(self, mock_parse, mock_extract, mock_check_output, mock_isfile):
        mock_isfile.return_value = True
        mock_check_output.return_value = b"manpage"
        mock_extract.return_value = ["word /* comment */, word2,", "/* comment */ word3;"]
        filename = f"{sigs.GUESTFS_ROOT}/usr/share/man/man2/func1.2.gz"

        assert sigs._get_manpage_function_signature("func1") is mock_parse.return_value
        mock_isfile.assert_called_once_with(filename)
        mock_check_output.assert_called_once_with(["zcat", filename])
        mock_extract.assert_called_once_with("func1", "manpage")
        mock_parse.assert_called_once_with("word , word2,  word3")

    @patch.object(sigs.os.path, "isfile")
    @patch.object(sigs.subprocess, "check_output")
    @patch.object(sigs, "_extract_manpage_function_definition")
    @patch.object(sigs, "_parse_manpage_function_signature")
    def test_get_manpage_function_signature_section3(self, mock_parse, mock_extract, mock_check_output, mock_isfile):
        mock_isfile.side_effect = [False, True]
        filename2 = f"{sigs.GUESTFS_ROOT}/usr/share/man/man2/func1.2.gz"
        filename3 = f"{sigs.GUESTFS_ROOT}/usr/share/man/man3/func1.3.gz"
        mock_check_output.return_value = b"manpage"
        mock_extract.return_value = ["word /* comment */, word2,", "/* comment */ word3;"]
        filename = f"{sigs.GUESTFS_ROOT}/usr/share/man/man3/func1.3.gz"

        assert sigs._get_manpage_function_signature("func1") is mock_parse.return_value
        assert mock_isfile.call_args_list == [
            call(filename2),
            call(filename3)
        ]
        mock_check_output.assert_called_once_with(["zcat", filename])
        mock_extract.assert_called_once_with("func1", "manpage")
        mock_parse.assert_called_once_with("word , word2,  word3")

    @patch.object(sigs.os.path, "isfile")
    @patch.object(sigs.subprocess, "check_output")
    @patch.object(sigs, "_extract_manpage_function_definition")
    @patch.object(sigs, "_parse_manpage_function_signature")
    def test_get_manpage_function_signature_not_found(self, mock_parse, mock_extract, mock_check_output, mock_isfile):
        mock_isfile.return_value = False
        filename2 = f"{sigs.GUESTFS_ROOT}/usr/share/man/man2/func1.2.gz"
        filename3 = f"{sigs.GUESTFS_ROOT}/usr/share/man/man3/func1.3.gz"

        assert sigs._get_manpage_function_signature("func1") is None
        assert mock_isfile.call_args_list == [
            call(filename2),
            call(filename3)
        ]
        mock_check_output.assert_not_called()
        mock_extract.assert_not_called()
        mock_parse.assert_not_called()

    @patch.object(sigs.os.path, "isfile")
    @patch.object(sigs.subprocess, "check_output")
    @patch.object(sigs, "_extract_manpage_function_definition")
    @patch.object(sigs, "_parse_manpage_function_signature")
    def test_get_manpage_function_signature_sp_error(self, mock_parse, mock_extract, mock_check_output, mock_isfile):
        mock_isfile.return_value = True
        filename = f"{sigs.GUESTFS_ROOT}/usr/share/man/man2/func1.2.gz"
        mock_check_output.side_effect = subprocess.CalledProcessError(10, "err")

        assert sigs._get_manpage_function_signature("func1") is None

        mock_check_output.assert_called_once_with(["zcat", filename])
        mock_extract.assert_not_called()
        mock_parse.assert_not_called()

    @patch.object(sigs.os.path, "isfile")
    @patch.object(sigs.subprocess, "check_output")
    @patch.object(sigs, "_extract_manpage_function_definition")
    @patch.object(sigs, "_parse_manpage_function_signature")
    def test_get_manpage_function_signature_no_def(self, mock_parse, mock_extract, mock_check_output, mock_isfile):
        mock_isfile.return_value = True
        mock_check_output.return_value = b"manpage"
        mock_extract.return_value = None
        filename = f"{sigs.GUESTFS_ROOT}/usr/share/man/man2/func1.2.gz"

        assert sigs._get_manpage_function_signature("func1") is None
        mock_isfile.assert_called_once_with(filename)
        mock_check_output.assert_called_once_with(["zcat", filename])
        mock_extract.assert_called_once_with("func1", "manpage")
        mock_parse.assert_not_called()

    @patch.object(sigs.subprocess, "Popen")
    def test_get_function_signatures(self, mock_popen):
        proc = mock_popen.return_value
        proc.communicate.return_value = (b"not a sig\nSIG:func1 0 0 4\nSIG:func2 1 2 3\n(gdb) exit\n", b"GDB")
        proc.returncode = 0
        assert sigs.get_function_signatures("libc.so", ["func1", "func2"]) == [
            "func1 0 0 4",
            "func2 1 2 3"
        ]
        mock_popen.assert_called_once_with(["gdb", "-q", "-x", sigs.__file__], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
        proc.communicate.assert_called_once_with(b"libc.so\nfunc1\nfunc2")

    @patch.object(sigs.subprocess, "Popen")
    def test_get_function_signatures_error(self, mock_popen):
        proc = mock_popen.return_value
        proc.communicate.return_value = (b"not a sig\nSIG:func1 0 0 4\nSIG:func2 1 2 3\n(gdb) exit\n", b"GDB")
        proc.returncode = 1
        assert sigs.get_function_signatures("libc.so", ["func1", "func2"]) == []
        mock_popen.assert_called_once_with(["gdb", "-q", "-x", sigs.__file__], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
        proc.communicate.assert_called_once_with(b"libc.so\nfunc1\nfunc2")
