from __future__ import print_function
import ConfigParser, StringIO
import os

import Kconfiglib.kconfiglib as kconfiglib

def kconfiglib_to_config_obj(kconfig):
    dict_ = {}
    for symbol in kconfig.get_symbols():
        dict_[symbol.name] = symbol.user_val

    return Config(dict_)

class Config(object):
    def __init__(self, config_dict):
        self.fallback = ''
        for key, val in config_dict.items():
            self.__dict__[key] = val

    def __getattribute__(self, name):
        """
        A non-recursive implementation of __getattribute__ to return the
        attribute if it exists, otherwise nothing.
        """
        own_dict = object.__getattribute__(self, '__dict__')
        if name == '__dict__':
            return own_dict

        return own_dict[name] if name in own_dict else own_dict['fallback']

    def __getitem__(self, name):
        return getattr(self, name)

    def __str__(self):
        return str(self.__dict__)

def get_base_kconfig():
    # Let SCons know we care about an exception.
    try:
        return kconfiglib.Config('Kconfig')
    except Exception as e:
        print('Unable to parse kconfig...')
        print()
        print(str(e))

def get_dotconfig():
    conf = get_base_kconfig()
    conf.load_config('.config')
    return conf

def allyesconfig(target, source, env):
    conf = get_base_kconfig()
    # Get a list of all symbols that are not in choices
    non_choice_syms = [sym for sym in conf.get_symbols() if
                       not sym.is_choice_symbol()]

    done = False
    while not done:
        done = True

        # Handle symbols outside of choices

        for sym in non_choice_syms:
            upper_bound = sym.get_upper_bound()

            # See corresponding comment for allnoconfig implementation
            if upper_bound is not None and \
               kconfiglib.tri_less(sym.get_value(), upper_bound):
                sym.set_user_value(upper_bound)
                done = False

        # Handle symbols within choices

        for choice in conf.get_choices():

            # Handle choices whose visibility allow them to be in "y" mode

            if choice.get_visibility() == "y":
                selection = choice.get_selection_from_defaults()
                if selection is not None and \
                   selection is not choice.get_user_selection():
                    selection.set_user_value("y")
                    done = False

            # Handle choices whose visibility only allow them to be in "m" mode.
            # This might happen if a choice depends on a symbol that can only be
            # "m" for example.

            elif choice.get_visibility() == "m":
                for sym in choice.get_symbols():
                    if sym.get_value() != "m" and \
                       sym.get_upper_bound() != "n":
                        sym.set_user_value("m")
                        done = False

    conf.write_config(".config")

def defconfig(target, source, env):
    conf = get_base_kconfig()

    if os.path.exists(".config"):
        conf.load_config(".config")
    else:
        defconfig = conf.get_defconfig_filename()
        #print(str(defconfig))
        if defconfig is not None:
            conf.load_config(defconfig)

    conf.write_config(".config")
