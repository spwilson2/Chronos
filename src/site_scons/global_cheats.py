#TODO: specify imports

from util import *
from SCons.Script import _SConscript, SConscript

local_globals = globals()
exports = {}

def export(dict_=None, **kwargs):

    if dict_:
        _SConscript.GlobalDict.update(dict_)
        for key, val in dict_.items():
            #globals()[key] = val
            local_globals[key] = val
    if kwargs:
        _SConscript.GlobalDict.update(**kwargs)
        for key, val in kwargs.items():
            local_globals[key] = val

def add_global_env(**kwargs):
    export(**kwargs)
    for key, val in kwargs.items():
        exports[key] = val


def include(sbuild):
    # TODO: Support a list of sbuilds as well.
    # TODO: Utility function to split lists into strings or return a single string
    # TODO:
    global kenv

    cloned_kenv = kenv.Clone()

    def call_with_kenv(kenv):
        SConscript(joinpath(sbuild,'Sbuild'), exports=exports.keys())
                #variant_dir=joinpath('build', sbuild))

    call_with_kenv(cloned_kenv)

def kenabled(option):
    global kconfig
    return 'y' in kconfig[option]
