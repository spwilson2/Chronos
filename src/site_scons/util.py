from __future__ import print_function
from SCons.Script import *
from os import path

def joinpath(base, *args):
    final_args = []
    if base is not None:
        final_args.append(base)

    for arg in args:
        if arg is not None:
            final_args.append(arg)

    if len(final_args) >= 1:
        return os.path.join(*final_args)
    else:
        return ''


def srcpath(relpath):
    """
    Take the relative path of a file as an arg and return the path of the
    file relative the source? or base? directory.
    """
    # Will be used for include paths.
    raise UnimplementedException()

def Phony(env = None, **kw):
    if not env: env = DefaultEnvironment()
    for target,action in kw.items():
        env.AlwaysBuild(env.Alias(target, [], action))

class UnimplementedException(Exception):
    pass
