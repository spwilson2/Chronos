from __future__ import print_function
from SCons.Script import ARGUMENTS

def enabled():
    return ARGUMENTS.get('DEBUG', False)

def debug_print(mod_name, *args, **kwargs):
    if enabled():
        print('{ module:', mod_name, '}', *args, **kwargs)
