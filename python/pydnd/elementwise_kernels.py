__all__ = ['abs', 'fabs', 'cabs', 'floor',
            'fmod', 'isnan', 'sin', 'cos']

import sys, ctypes, dnd_ctypes

if sys.platform == 'win32':
    # integer absolute value
    abs = ctypes.cdll.msvcrt.abs
    abs.restype = ctypes.c_int
    abs.argtypes = [ctypes.c_int]

    # float absolute value
    fabs = ctypes.cdll.msvcrt.fabs
    fabs.restype = ctypes.c_double
    fabs.argtypes = [ctypes.c_double]

    # complex absolute value
    cabs = ctypes.cdll.msvcrt._cabs
    cabs.restype = ctypes.c_double
    cabs.argtypes = [dnd_ctypes.c_complex_float64]

    # floor
    floor = ctypes.cdll.msvcrt.floor
    floor.restype = ctypes.c_double
    floor.argtypes = [ctypes.c_double]

    # ceil
    ceil = ctypes.cdll.msvcrt.ceil
    ceil.restype = ctypes.c_double
    ceil.argtypes = [ctypes.c_double]

    # fmod
    fmod = ctypes.cdll.msvcrt.fmod
    fmod.restype = ctypes.c_double
    fmod.argtypes = [ctypes.c_double, ctypes.c_double]

    # isnan
    isnan = ctypes.cdll.msvcrt._isnan
    isnan.restype = ctypes.c_int
    isnan.argtypes = [ctypes.c_double]

    # pow
    pow = ctypes.cdll.msvcrt.pow
    pow.restype = ctypes.c_double
    pow.argtypes = [ctypes.c_double, ctypes.c_double]

    # square root
    sqrt = ctypes.cdll.msvcrt.sqrt
    sqrt.restype = ctypes.c_double
    sqrt.argtypes = [ctypes.c_double]

    # exp
    exp = ctypes.cdll.msvcrt.exp
    exp.restype = ctypes.c_double
    exp.argtypes = [ctypes.c_double]

    # log
    log = ctypes.cdll.msvcrt.log
    log.restype = ctypes.c_double
    log.argtypes = [ctypes.c_double]

    # log10
    log10 = ctypes.cdll.msvcrt.log10
    log10.restype = ctypes.c_double
    log10.argtypes = [ctypes.c_double]

    # sine
    sin = ctypes.cdll.msvcrt.sin
    sin.restype = ctypes.c_double
    sin.argtypes = [ctypes.c_double]

    # cosine
    cos = ctypes.cdll.msvcrt.cos
    cos.restype = ctypes.c_double
    cos.argtypes = [ctypes.c_double]

    # tangent
    tan = ctypes.cdll.msvcrt.tan
    tan.restype = ctypes.c_double
    tan.argtypes = [ctypes.c_double]

    # arc sine
    asin = ctypes.cdll.msvcrt.asin
    asin.restype = ctypes.c_double
    asin.argtypes = [ctypes.c_double]

    # arc cosine
    acos = ctypes.cdll.msvcrt.acos
    acos.restype = ctypes.c_double
    acos.argtypes = [ctypes.c_double]

    # arc tan
    atan = ctypes.cdll.msvcrt.atan
    atan.restype = ctypes.c_double
    atan.argtypes = [ctypes.c_double]

    # arc tan2
    atan2 = ctypes.cdll.msvcrt.atan2
    atan2.restype = ctypes.c_double
    atan2.argtypes = [ctypes.c_double, ctypes.c_double]

    # hyperbolic sine
    sinh = ctypes.cdll.msvcrt.sinh
    sinh.restype = ctypes.c_double
    sinh.argtypes = [ctypes.c_double]

    # hyperbolic cosine
    cosh = ctypes.cdll.msvcrt.cosh
    cosh.restype = ctypes.c_double
    cosh.argtypes = [ctypes.c_double]

    # ldexp
    ldexp = ctypes.cdll.msvcrt.ldexp
    ldexp.restype = ctypes.c_double
    ldexp.argtypes = [ctypes.c_double, ctypes.c_int]

    # isfinite
    isfinite = ctypes.cdll.msvcrt._finite
    ldexp.restype = ctypes.c_int
    ldexp.argtypes = [ctypes.c_double]

    # nextafter
    nextafter = ctypes.cdll.msvcrt._nextafter
    nextafter.restype = ctypes.c_double
    nextafter.argtypes = [ctypes.c_double, ctypes.c_double]

# TODO: Add kernels for other platforms