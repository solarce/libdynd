//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include "ctypes_interop.hpp"
#include "utility_functions.hpp"

using namespace std;
using namespace dnd;
using namespace pydnd;

ctypes_info pydnd::ctypes;


void pydnd::init_ctypes_interop()
{
    memset(&ctypes, 0, sizeof(ctypes));

    // The C _ctypes module
    ctypes._ctypes = PyImport_ImportModule("_ctypes");
    if (ctypes._ctypes == NULL) {
        throw runtime_error("Could not import module _ctypes");
    }

    // The internal type objects used by ctypes
    ctypes.PyCStructType_Type = PyObject_GetAttrString(ctypes._ctypes, "Structure");
    // _ctypes doesn't expose PyCData_Type, but we know it's the base class of PyCStructType_Type
    ctypes.PyCData_Type = (PyObject *)((PyTypeObject *)ctypes.PyCStructType_Type)->tp_base;
    ctypes.UnionType_Type = PyObject_GetAttrString(ctypes._ctypes, "Union");
    ctypes.PyCPointerType_Type = PyObject_GetAttrString(ctypes._ctypes, "_Pointer");
    ctypes.PyCArrayType_Type = PyObject_GetAttrString(ctypes._ctypes, "Array");
    ctypes.PyCSimpleType_Type = PyObject_GetAttrString(ctypes._ctypes, "_SimpleCData");
    ctypes.PyCFuncPtrType_Type = PyObject_GetAttrString(ctypes._ctypes, "CFuncPtr");

    if (PyErr_Occurred()) {
        Py_XDECREF(ctypes._ctypes);

        Py_XDECREF(ctypes.PyCData_Type);
        Py_XDECREF(ctypes.PyCStructType_Type);
        Py_XDECREF(ctypes.UnionType_Type);
        Py_XDECREF(ctypes.PyCPointerType_Type);
        Py_XDECREF(ctypes.PyCArrayType_Type);
        Py_XDECREF(ctypes.PyCSimpleType_Type);
        Py_XDECREF(ctypes.PyCFuncPtrType_Type);

        memset(&ctypes, 0, sizeof(ctypes));
        throw std::runtime_error("Error initializing ctypes C-level data for low level interop");
    }
}


ctypes_calling_convention pydnd::get_ctypes_calling_convention(PyCFuncPtrObject* cfunc)
{
    // This is the internal StgDictObject "flags" attribute, which is
    // custom-placed in the typeobject's dict by ctypes.
    pyobject_ownref flags_obj(PyObject_GetAttrString((PyObject *)Py_TYPE(cfunc), "_flags_"));

    long flags = PyInt_AsLong(flags_obj);
    if (flags == -1 && PyErr_Occurred()) {
        throw std::runtime_error("Error getting ctypes function flags");
    }

    if (flags&0x02) { // 0x02 is FUNCFLAG_HRESULT
        throw std::runtime_error("Functions returning an HRESULT are not supported");
    }

    //if (flags&0x04) { // 0x04 is FUNCFLAG_PYTHONAPI, may need special handling
    //}

    if (flags&0x08) { // 0x08 is FUNCFLAG_USE_ERRNO
        throw std::runtime_error("Functions using errno are not yet supported");
    }

    if (flags&0x10) { // 0x10 is FUNCFLAG_USE_LASTERROR
        throw std::runtime_error("Functions using lasterror are not yet supported");
    }

    // Only on Win32 are non-CDECL calling conventions supported
#ifdef _WIN32
    if (cfunc->index) {
        throw std::runtime_error("COM functions are not supported");
    }
    if (flags&0x01) { // 0x01 is FUNCFLAG_CDECL from cpython's internal ctypes.h
        return cdecl_callconv;
    } else {
        return win32_stdcall_callconv;
    }
#else
    return cdecl_callconv;
#endif
}



dnd::dtype pydnd::dtype_from_ctypes_cdatatype(PyObject *d)
{
    if (!PyObject_IsSubclass(d, ctypes.PyCData_Type)) {
        throw runtime_error("requested a dtype from a ctypes c data type, but the given object has the wrong type");
    }

    // The simple C data types
    if (PyObject_IsSubclass(d, ctypes.PyCSimpleType_Type)) {
        char *proto_str = NULL;
        Py_ssize_t proto_len = 0;
        pyobject_ownref proto(PyObject_GetAttrString(d, "_type_"));
        if (PyString_AsStringAndSize(proto, &proto_str, &proto_len) < 0 ||
                            proto_len != 1) {
            throw std::runtime_error("invalid ctypes type");
        }

        switch (proto_str[0]) {
        case 'b':
            return make_dtype<int8_t>();
        case 'B':
            return make_dtype<uint8_t>();
        case 'c': // TODO: make this a fixed sized string of size 1 instead of a number
            return make_dtype<char>();
        case 'd':
            return make_dtype<double>();
        case 'f':
            return make_dtype<float>();
        case 'h':
            return make_dtype<int16_t>();
        case 'H':
            return make_dtype<uint16_t>();
        case 'i':
            return make_dtype<int32_t>();
        case 'I':
            return make_dtype<uint32_t>();
        case 'l':
            return make_dtype<long>();
        case 'L':
            return make_dtype<unsigned long>();
        case 'q':
            return make_dtype<int64_t>();
        case 'Q':
            return make_dtype<uint64_t>();
        default: {
            stringstream ss;
            ss << "The ctypes type code '" << proto_str[0] << "' cannot be converted to a dnd::dtype";
            throw runtime_error(ss.str());
            }
        }
    }

    throw runtime_error("Ctypes type object is not supported by dnd::dtype");
}