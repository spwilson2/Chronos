# TODO: Limit after development
from os import getenv

from util import *
from kconf_util import *
from global_cheats import *
from SCons.Script import Import as __Import
from SCons.Script import ARGUMENTS
import debug

def Init(*args, **kwargs):
    __Import('kenv')
    __Import(*args)

def build_path():
    # Take a build directory
    return ARGUMENTS.get('O', '')

def include_path(path):
    return Dir(path)

def add_flags(env, **kwargs):
    for key, val in kwargs.items():
        if not is_list(val):
            kwargs[key] = [val]

    env.MergeFlags(kwargs)

def add_include_path(env, paths):
    add_include = lambda path : add_flags(env, CPPPATH=Dir(path))
    run_list_or_single(paths, add_include)

def run_list_or_single(list_or_item, fn):
    if is_list(list_or_item):
        for item in list_or_item:
            run_list_or_single(item, fn)
    else:
        fn(list_or_item)

from collections import UserList
def is_list(item):
    type_ = type(item)
    return type_ is list or type_ is UserList

def is_string(item):
    type_ = type(item)
    return type_ is str or type_ is type('')

def InitSbuild():
    export(add_flags=add_flags)

def EnvExport(**kwargs):
    for key, val in kwargs.items():
        os.environ[key] = val

def import_tool(path, tool):
    """Import a tool from a directory path"""
    include(path)
    Import(tool)
    return globals()[tool]

__tools = {}

TOOL_PROGRAMS = []

def Tool(target, sources, usage=None ,env=None):
    """Define a tool that is used to generate sources."""

    if target in __tools:
        if debug.enabled():
            debug.debug_print(__name__, ': ', target, 'has already been built, '
                    'returning: ', str(__tools[target]), 'as Tool.')
        return __tools[target]


    if env is None:
        env = local_globals['host_env']

    if sources is None:
        debug.debug_print(__name__, ': ', target, 'listed `None` as a source,'
                'assuming source is already exectuable')
        tool = File(target)
    else:
        tool = env.Program(target, sources)[0]
        TOOL_PROGRAMS.append(tool)

    if usage is None:
        usage = tool.abspath + ' $SOURCE $TARGET'

    def depends_emitter(target, source, env):
        """
        An emitter than automatically makes targets build with this tool,
        depend on the tool.
        """
        env.Depends(target, tool)
        return (target, source)

    tool_builder = Builder(action = usage, emitter = depends_emitter)

    # Add the $TOOLPATH variable for easier calling of tools
    env = env.Clone(TOOLPATH=tool.abspath)

    env['BUILDERS'][target] = tool_builder

    # Add the target to our local globals so we can `Export` it.
    tool_object = getattr(env, target)
    globals()[target] = tool_object

    # Add the tool to build targets, so we don't rebuild when importing.
    __tools[target] = tool_object

    # Export the tool so we can Import it when we need to import_tool
    Export(target)


class Target(object):
    def __init__(self):
        self.options = {}
        pass

    def __getitem__(self, key):
        if key not in self.options:
            self.options[key] = []
        return self.options[key]

    def __setitem__(self, key, item):
        self.options[key] = item
