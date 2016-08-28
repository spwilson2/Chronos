from SCons.Script import *
from os import path

joinpath = os.path.join


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
