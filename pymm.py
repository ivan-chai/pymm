# This file was automatically generated by SWIG (http://www.swig.org).
# Version 3.0.7
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.





from sys import version_info
if version_info >= (2, 6, 0):
    def swig_import_helper():
        from os.path import dirname
        import imp
        fp = None
        try:
            fp, pathname, description = imp.find_module('_pymm', [dirname(__file__)])
        except ImportError:
            import _pymm
            return _pymm
        if fp is not None:
            try:
                _mod = imp.load_module('_pymm', fp, pathname, description)
            finally:
                fp.close()
            return _mod
    _pymm = swig_import_helper()
    del swig_import_helper
else:
    import _pymm
del version_info
try:
    _swig_property = property
except NameError:
    pass  # Python < 2.2 doesn't have 'property'.


def _swig_setattr_nondynamic(self, class_type, name, value, static=1):
    if (name == "thisown"):
        return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'SwigPyObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name, None)
    if method:
        return method(self, value)
    if (not static):
        if _newclass:
            object.__setattr__(self, name, value)
        else:
            self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)


def _swig_setattr(self, class_type, name, value):
    return _swig_setattr_nondynamic(self, class_type, name, value, 0)


def _swig_getattr_nondynamic(self, class_type, name, static=1):
    if (name == "thisown"):
        return self.this.own()
    method = class_type.__swig_getmethods__.get(name, None)
    if method:
        return method(self)
    if (not static):
        return object.__getattr__(self, name)
    else:
        raise AttributeError(name)

def _swig_getattr(self, class_type, name):
    return _swig_getattr_nondynamic(self, class_type, name, 0)


def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except:
        strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

try:
    _object = object
    _newclass = 1
except AttributeError:
    class _object:
        pass
    _newclass = 0



_pymm.EFF_UNK_STREAM_swigconstant(_pymm)
EFF_UNK_STREAM = _pymm.EFF_UNK_STREAM

_pymm.EFF_AUDIO_STREAM_swigconstant(_pymm)
EFF_AUDIO_STREAM = _pymm.EFF_AUDIO_STREAM

_pymm.EFF_VIDEO_STREAM_swigconstant(_pymm)
EFF_VIDEO_STREAM = _pymm.EFF_VIDEO_STREAM
class TFFmpegReader(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, TFFmpegReader, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, TFFmpegReader, name)
    __repr__ = _swig_repr

    def __init__(self, fname):
        this = _pymm.new_TFFmpegReader(fname)
        try:
            self.this.append(this)
        except:
            self.this = this
    __swig_destroy__ = _pymm.delete_TFFmpegReader
    __del__ = lambda self: None

    def read(self, stream, sampNum):
        return _pymm.TFFmpegReader_read(self, stream, sampNum)

    def close(self):
        return _pymm.TFFmpegReader_close(self)

    def __len__(self):
        return _pymm.TFFmpegReader___len__(self)

    def info(self, stream):
        return _pymm.TFFmpegReader_info(self, stream)

    def size(self, stream):
        return _pymm.TFFmpegReader_size(self, stream)
TFFmpegReader_swigregister = _pymm.TFFmpegReader_swigregister
TFFmpegReader_swigregister(TFFmpegReader)

# This file is compatible with both classic and new-style classes.


