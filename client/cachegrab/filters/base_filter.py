# This file is part of the Cachegrab GUI.
#
# Copyright (C) 2017 NCC Group
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Cachegrab.  If not, see <http://www.gnu.org/licenses/>.

# Version 0.1.0
# Keegan Ryan, NCC Group

import wx

class FilterGui(wx.Panel):
    """Basic GUI for a single filter."""

    def __init__(self, parent, filt, delete_func, up_func, down_func):
        """Initializer"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SIMPLE)

        # Initialize controller
        self.filt = filt

        # Initialize view
        self.sizer = wx.BoxSizer(wx.VERTICAL)

        statusbar = wx.BoxSizer(wx.HORIZONTAL)
        self.name = wx.StaticText(self, wx.ID_ANY, self.filt.name)
        font = self.name.GetFont()
        font.SetWeight(wx.BOLD)
        self.name.SetFont(font)

        self.btn_en = wx.Button(self, wx.ID_ANY, "", style=wx.BU_EXACTFIT)
        btn_del = wx.Button(self, wx.ID_ANY, "Delete", style=wx.BU_EXACTFIT)
        self.btn_up = wx.Button(self, wx.ID_UP, style=wx.BU_EXACTFIT)
        self.btn_down = wx.Button(self, wx.ID_DOWN, style=wx.BU_EXACTFIT)

        statusbar.Add(self.name, 1, wx.ALIGN_CENTER)
        statusbar.Add(self.btn_en, 0, wx.ALIGN_CENTER)
        statusbar.Add(btn_del, 0, wx.ALIGN_CENTER)
        statusbar.Add(self.btn_up, 0, wx.ALIGN_CENTER)
        statusbar.Add(self.btn_down, 0, wx.ALIGN_CENTER)

        self.sizer.Add(statusbar, 0, wx.EXPAND | wx.ALL, 5)
        self.SetSizer(self.sizer)
        self.sizer.SetSizeHints(self)

        # Events
        self.btn_en.Bind(wx.EVT_BUTTON, self.onclick_enable)
        btn_del.Bind(wx.EVT_BUTTON, delete_func)
        self.btn_up.Bind(wx.EVT_BUTTON, up_func)
        self.btn_down.Bind(wx.EVT_BUTTON, down_func)

        # Initial data
        self.draw_enable()

    def onclick_enable(self, evt):
        """The enable button was clicked."""
        self.filt.enabled = not self.filt.enabled
        self.draw_enable()

    def draw_enable(self):
        """Draw the enable button."""
        if self.filt.enabled:
            self.btn_en.SetLabel("Disable")
        else:
            self.btn_en.SetLabel("Enable")
        font = self.name.GetFont()
        font.SetStrikethrough(not self.filt.enabled)
        self.name.SetFont(font)

class Filter(object):
    """Takes a generator of Samples, filters them, and returns a generator"""

    name = "Default Filter"
    typename = "Default_Filter"
    enabled = True

    def filter(self, sample):
        """Filter and return a single Sample."""
        raise NotImplementedError

    def filter_many(self, samples):
        """Filter and return several samples at once.
        
        This may be overridden in the case where filtering one sample
        depends on the other samples that are passed in. Otherwise the
        samples are filtered lazily."""
        for s in samples:
            if self.enabled:
                yield self.filter(s.copy())
            else:
                yield s.copy()

    def save(self):
        """Return a object representing the sink's internal state.

        The object will be later encoded into JSON."""
        desc = {
            "type": self.typename,
            "enabled": self.enabled,
        }
        return desc

    def load(self, desc):
        """Take the object and set the sink's internal state.

        Returns a reference to the current object."""
        if desc["type"] != self.typename:
            raise TypeError("Unexpected Sink type %s." % desc["type"])

        self.enabled = desc["enabled"]

        return self

