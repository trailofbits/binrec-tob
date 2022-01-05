#include "binrec_link.hpp"
#include "link_error.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/ManagedStatic.h>

extern "C" {

#include <Python.h>


static void raise_error(const llvm::Error &error)
{
    std::string msg;
    llvm::raw_string_ostream error_stream(msg);
    PyObject *error_args;

    error_stream << error;

    error_args = Py_BuildValue("(is)", ECHILD, msg.c_str());
    PyErr_SetObject(PyExc_ChildProcessError, error_args);
}

PyDoc_STRVAR(
    binrec_link__doc__,
    "link(binary_filename: str, recovered_filename: str, runtime_library: str, "
    "linker_script: str, destination: str) -> None\n\n"
    "Recover and link a binary from a merged trace.\n\n"
    ":param binary_filename: the S2E binary filename, which must match the merged "
    "trace directory, ``s2e-out-{binary_filename}``\n"
    ":param recovered_filename: the recovered object filename, typically "
    "``{trace_dir}/recovered.o``\n"
    ":param runtime_library: the binrec runtime library filename, typically "
    "``{BINREC_LIB}/libbinrec_rt.a``\n"
    ":param linker_script: LD linker script filename, typically "
    "``{BINREC_LINK_LD}/i386.ld``\n"
    ":param destination: the output filename for the recovered binary, typically "
    "``{trace_dir}/recovered``\n");
static PyObject *binrec_link(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] = {
        "binary_filename",
        "recovered_filename",
        "runtime_library",
        "linker_script",
        "destination",
        NULL};

    const char *binary_filename = NULL;
    const char *recovered_filename = NULL;
    const char *runtime_library = NULL;
    const char *linker_script = NULL;
    const char *destination = NULL;
    binrec::LinkContext ctx;

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "sssss",
            const_cast<char **>(kwlist),
            &binary_filename,
            &recovered_filename,
            &runtime_library,
            &linker_script,
            &destination))
    {
        return NULL;
    }

    llvm::llvm_shutdown_obj y;

    if (auto ec = llvm::sys::fs::createUniqueDirectory("binrec_link", ctx.work_dir)) {
        PyObject *error_args = Py_BuildValue("(iss)", ec.value(), ec.message().c_str(), "/tmp");
        PyErr_SetObject(PyExc_OSError, error_args);
        return NULL;
    }

    auto binary = llvm::object::createBinary(binary_filename);
    if (auto err = binary.takeError()) {
        raise_error(err);
        return NULL;
    }

    ctx.original_binary = std::move(binary.get());
    ctx.recovered_filename = recovered_filename;
    ctx.librt_filename = runtime_library;
    ctx.ld_script_filename = linker_script;
    ctx.output_filename = destination;

    if (auto err = binrec::run_link(ctx)) {
        raise_error(err);
        binrec::cleanup_link(ctx);

        return NULL;
    }

    binrec::cleanup_link(ctx);

    Py_RETURN_NONE;
}

static PyMethodDef LinkMethods[] = {
    {"link", (PyCFunction)binrec_link, METH_VARARGS | METH_KEYWORDS, binrec_link__doc__},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef link_module = {
    PyModuleDef_HEAD_INIT,
    "_binrec_link",                                  /* name of module */
    "Python wrapper for the binrec_link executable", /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global
           variables. */
    LinkMethods};

PyMODINIT_FUNC PyInit__binrec_link(void)
{
    return PyModule_Create(&link_module);
}
}
