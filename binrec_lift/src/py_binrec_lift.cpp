#include <llvm/ADT/Triple.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CommandLine.h>
//#include <llvm/ADT/StringMap.h>
#include "binrec_lift.hpp"
#include "lift_context.hpp"
#include <llvm/Analysis/AliasAnalysis.h>


extern "C" {

#include <Python.h>

/**
 * Set the LLVM memssa check limit for the current lift operation.
 */
static void set_memssa_check_limit(int limit)
{
    llvm::StringMap<llvm::cl::Option *> &opts = llvm::cl::getRegisteredOptions();
    auto option = (llvm::cl::opt<unsigned> *)opts["memssa-check-limit"];
    option->setValue(limit);
}

/**
 * Reset LLVM command line arguments.
 */
static void reset_llvm_options()
{
    llvm::StringMap<llvm::cl::Option *> &opts = llvm::cl::getRegisteredOptions();
    auto option = (llvm::cl::opt<unsigned> *)opts["memssa-check-limit"];
    option->setValue(option->getDefault().getValue());
}

/**
 * Helper class that properly sets up LLVM prior to a lift opertion and then cleans up
 * when the operation completes.
 */
class BinrecCallState {
public:
    const char *working_dir;
    unsigned memssa_check_limit;
    char pwd[PATH_MAX];
    bool good;

    BinrecCallState(const char *working_dir, unsigned memssa_check_limit) :
            working_dir(working_dir),
            memssa_check_limit(memssa_check_limit)
    {
        reset_llvm_options();
        if (memssa_check_limit) {
            set_memssa_check_limit(memssa_check_limit);
        }

        good = set_working_dir() == 0;
    }

    ~BinrecCallState()
    {
        reset_working_dir();
    }

    /**
     * Set the current working directory for the duration of the lift operation.
     *
     * @returns 0 on success and -1, with a Python exception, on error.
     */
    int set_working_dir()
    {
        if (working_dir) {
            getcwd(pwd, PATH_MAX);
            if (chdir(working_dir)) {
                PyErr_SetFromErrnoWithFilename(PyExc_OSError, working_dir);
                return -1;
            }
        }
        return 0;
    }

    void reset_working_dir()
    {
        if (working_dir && good) {
            chdir(pwd);
        }
    }
};


/**
 * Run a lift operation with the provided context.
 *
 * @returns 0 on success or -1, with a Python exception, on error.
 */
static int run_lift_operation(binrec::LiftContext &ctx)
{
    int status;

    try {
        binrec::run_lift(ctx);
        status = 0;
    } catch (std::runtime_error &err) {
        // TODO
        PyErr_SetString(PyExc_RuntimeError, err.what());
        status = -1;
    }

    return status;
}

PyDoc_STRVAR(
    link_prep_1__doc__,
    "link_prep_1(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Perform a first-pass bitcode preparation for linking on the trace filename.\n\n"
    ":param trace_filename: the bitcode catpured trace to prepare\n"
    ":param destination: the output bitcode file\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *link_prep_1(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    llvm::LLVMContext llvm_context;
    BinrecCallState state{working_dir, memssa_check_limit};

    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.link_prep_1 = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    link_prep_2__doc__,
    "link_prep_2(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Perform a second-pass bitcode prepation for linking on the trace filename.\n\n"
    ":param trace_filename: the bitcode catpured trace to prepare\n"
    ":param destination: the output bitcode file\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *link_prep_2(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.link_prep_2 = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    clean__doc__,
    "clean(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Clean the bitcode trace. This method produces three output files:\n"
    "  - ``{destination}.bc`` - cleaned bitcode\n"
    "  - ``{destination}.ll`` - cleaned LLVM IR\n"
    "  - ``{destination}-memssa.ll`` - cleaned LLVM IR run through MemorySSA analysis\n\n"
    ":param str trace_filename: the bitcode catpured trace to prepare\n"
    ":param str destination: the output file basename\n"
    ":param str working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param str memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *clean(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.clean = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    lift__doc__,
    "lift(trace_filename: str, destination: str, working_dir: str = None, "
    "clean_names: bool = False, skip_link: bool = False, trace_calls: bool = False, "
    "memssa_check_link: int = None) -> None\n\n"
    "Lift bitcode to an LLVM module. This function outputs multiple files:\n"
    " - ``{destination}.bc`` - lifted bitcode\n"
    " - ``{destination}.ll`` - lifted LLVM IR\n"
    " - ``{destination}-memssa.ll`` - lifted LLVM IR run through MemorySSA analysis\n"
    " - ``rfuncs`` - extracted functions\n\n"
    ":param trace_filename: the bitcode catpured trace to lift\n"
    ":param destination: the output file basename\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param clean_names: clean symbol names\n"
    ":param skip_link: do not lift dynamic symbols\n"
    ":param trace_calls: trace calls and register values of recovered functions\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *lift(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] = {
        "trace_filename",
        "destination",
        "working_dir",
        "clean_names",
        "skip_link",
        "trace_calls",
        "memssa_check_limit",
        NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    int skip_link = 0;
    int clean_names = 0;
    int trace_calls = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sIppp",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit,
            &clean_names,
            &skip_link,
            &trace_calls))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.lift = true;
    ctx.clean_names = (bool)clean_names;
    ctx.skip_link = (bool)skip_link;
    ctx.trace_calls = (bool)trace_calls;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    optimize__doc__,
    "optmize(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Optmize lifted bitcode. These function outputs multiple files:\n"
    " - ``{destination}.bc`` - optimized bitcode\n"
    " - ``{destination}.ll`` - optimized LLVM IR\n"
    " - ``{destination}-memssa.ll`` - optimized LLVM IR run through MemorySSA analysis\n\n"
    ":param trace_filename: the bitcode catpured trace to prepare\n"
    ":param destination: the output bitcode file\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *optimize(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.optimize = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    optimize_better__doc__,
    "optmize_better(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Optmize lifted bitcode (better than :func:`optimize`). These function outputs "
    "multiple files:\n"
    " - ``{destination}.bc`` - optimized bitcode\n"
    " - ``{destination}.ll`` - optimized LLVM IR\n"
    " - ``{destination}-memssa.ll`` - optimized LLVM IR run through MemorySSA analysis\n\n"
    ":param trace_filename: the bitcode catpured trace to prepare\n"
    ":param destination: the output bitcode file\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *optimize_better(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.optimize_better = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(
    compile_prep__doc__,
    "compile_prep(trace_filename: str, destination: str, working_dir: str = None, "
    "memssa_check_link: int = None) -> None\n\n"
    "Prepare a trace for compilation to an object file. This function outputs "
    "multiple files:\n"
    " - ``{destination}.bc`` - preped bitcode\n"
    " - ``{destination}.ll`` - preped LLVM IR\n"
    " - ``{destination}-memssa.ll`` - preped LLVM IR run through MemorySSA analysis\n\n"
    ":param trace_filename: the bitcode catpured trace to prepare\n"
    ":param destination: the output bitcode file\n"
    ":param working_dir: the working directory, which is typically the catpure trace "
    "directory\n"
    ":param memssa_check_limit: the maximum number of stores/phis MemorySSA will consider "
    "trying to walk past (default = 100)\n");
static PyObject *compile_prep(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] =
        {"trace_filename", "destination", "working_dir", "memssa_check_limit", NULL};

    const char *trace_filename = NULL;
    const char *destination = NULL;
    const char *working_dir = NULL;
    unsigned int memssa_check_limit = 0;
    binrec::LiftContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|sI",
            const_cast<char **>(kwlist),
            &trace_filename,
            &destination,
            &working_dir,
            &memssa_check_limit))
    {
        return NULL;
    }

    BinrecCallState state{working_dir, memssa_check_limit};
    if (!state.good) {
        return NULL;
    }

    ctx.trace_filename = trace_filename;
    ctx.destination = destination;
    ctx.compile = true;

    int status = run_lift_operation(ctx);
    if (status) {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyMethodDef LiftMethods[] = {
    {"link_prep_1", (PyCFunction)link_prep_1, METH_VARARGS | METH_KEYWORDS, link_prep_1__doc__},
    {"link_prep_2", (PyCFunction)link_prep_2, METH_VARARGS | METH_KEYWORDS, link_prep_2__doc__},
    {"clean", (PyCFunction)clean, METH_VARARGS | METH_KEYWORDS, clean__doc__},
    {"lift", (PyCFunction)lift, METH_VARARGS | METH_KEYWORDS, lift__doc__},
    {"optimize", (PyCFunction)optimize, METH_VARARGS | METH_KEYWORDS, optimize__doc__},
    {"optimize_better",
     (PyCFunction)optimize_better,
     METH_VARARGS | METH_KEYWORDS,
     optimize_better__doc__},
    {"compile_prep", (PyCFunction)compile_prep, METH_VARARGS | METH_KEYWORDS, compile_prep__doc__},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef lift_module = {
    PyModuleDef_HEAD_INIT,
    "_binrec_lift",                                  /* name of module */
    "Python wrapper for the binrec_lift executable", /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global
           variables. */
    LiftMethods};

PyMODINIT_FUNC PyInit__binrec_lift(void)
{
    return PyModule_Create(&lift_module);
}
}
