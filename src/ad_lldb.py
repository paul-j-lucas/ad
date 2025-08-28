##
#       ad -- ASCII dump
#       src/ad_lldb.py
#
#       Copyright (C) 2020-2025  Paul J. Lucas
#
#       This program is free software: you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation, either version 3 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

import lldb

###############################################################################

def __lldb_init_module(debugger, internal_dict):
    cmd_prefix = 'type summary add -F ' + __name__
    debugger.HandleCommand(cmd_prefix + '.show_sname_scope_t sname_scope_t')
    debugger.HandleCommand(cmd_prefix + '.show_sname_t sname_t')


def is_null(ptr):
    """Gets whether the SBValue is a NULL pointer."""
    return not ptr.IsValid() or ptr.GetValueAsUnsigned() == 0


def show_sname_scope_t(sname_scope, internal_dict):
    """Pretty printer for a sname_scope_t."""
    target = lldb.debugger.GetSelectedTarget()
    sname_scope_ptr_t = target.FindFirstType('sname_scope_t').GetPointerType()
    rv = ""

    void_data_ptr = sname_scope.GetChildMemberWithName('data')
    if not is_null(void_data_ptr):
        sname_scope_ptr = void_data_ptr.Cast(sname_scope_ptr_t)
        name_ptr = sname_scope_ptr.GetChildMemberWithName('name')
        if not is_null(name_ptr):
            rv = name_ptr.GetSummary().strip('"')

    return '"' + rv + '"'


def show_sname_t(sname, internal_dict):
    """Pretty printer for a sname_t, e.g. S::T."""
    target = lldb.debugger.GetSelectedTarget()
    sname_scope_ptr_t = target.FindFirstType('sname_scope_t').GetPointerType()
    colon2 = False
    rv = ""

    head = sname.GetChildMemberWithName('head')
    for slist_node_ptr in head.linked_list_iter('next'):
        void_data_ptr = slist_node_ptr.GetChildMemberWithName('data')
        if not is_null(void_data_ptr):
            sname_scope_ptr = void_data_ptr.Cast(sname_scope_t)
            name_ptr = sname_scope_ptr.GetChildMemberWithName('name')
            if not is_null(name_ptr):
                if colon2:
                    rv += '::'
                else:
                    colon2 = True
                rv += name_ptr.GetSummary().strip('"')

    return '"' + rv + '"'


###############################################################################
# vim:set et sw=4 ts=4:
