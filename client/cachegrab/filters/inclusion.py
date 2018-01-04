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

import numpy as np
import wx
import wx.lib.agw.floatspin as FS

import cachegrab
from cachegrab.filters import Filter, FilterGui
from cachegrab.data import Trace

class InclusionFilterGui(FilterGui):
    """GUI for inclusion filter"""

    def __init__(self, parent, *args):
        """Initialize"""
        FilterGui.__init__(self, parent, *args)

        self.trace = wx.TextCtrl(self, wx.ID_ANY)
        self._add_field("Trace to Filter", self.trace, small=True)

        self.include = wx.TextCtrl(self, wx.ID_ANY)
        self._add_field("Sets Included (CSV)", self.include)

        self.exclude = wx.TextCtrl(self, wx.ID_ANY)
        self._add_field("Sets Excluded (CSV)", self.exclude)

        self.SetSizer(self.sizer)
        self.sizer.SetSizeHints(self)

        # Initialize events
        self.trace.Bind(wx.EVT_TEXT, self.onchange_params)
        self.exclude.Bind(wx.EVT_TEXT, self.onchange_params)
        self.include.Bind(wx.EVT_TEXT, self.onchange_params)

        self.draw()

    def _add_field(self, label, item, small=False):
        """Add a label/element pair to the UI"""
        szr = wx.BoxSizer(wx.HORIZONTAL)
        lbl = wx.StaticText(self, wx.ID_ANY, label)
        szr.Add(lbl, 1 if small else 0, wx.ALIGN_CENTER)
        szr.Add((15, 15), 0, wx.EXPAND)
        szr.Add(item, 0 if small else 1, wx.ALIGN_RIGHT)
        self.sizer.Add(szr,
                       0,
                       wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT,
                       5)

    def onchange_params(self, evt):
        """The params were changed"""
        self.filt.target_trace = self.trace.GetValue()
        self.filt.include_sets = self.include.GetValue()
        self.filt.exclude_sets = self.exclude.GetValue()

    def draw(self):
        """Draw all data"""
        self.trace.ChangeValue(self.filt.target_trace)
        self.include.ChangeValue(self.filt.include_sets)
        self.exclude.ChangeValue(self.filt.exclude_sets)

class InclusionFilter(Filter):
    """Remove samples from the trace that fail to include or exclude sets.

    This filter focuses on a single trace and finds time indices that
    lack an indicator within a set, or have one when it shouldn't. It then
    removes those times from all traces.
    """

    name = "Inclusion/Exclusion"
    typename = "Inclusion_Exclusion"

    def __init__(self):
        """Initializer"""
        Filter.__init__(self)

        self.target_trace = ""
        self.include_sets = ""
        self.exclude_sets = ""

    def filter(self, sample):
        """Filter and return a single Sample"""
        if self.target_trace not in sample.get_trace_names():
            return sample

        inc = []
        try:
            if len(self.include_sets) > 0:
                inc = [int(s.strip()) for s in self.include_sets.split(",")]
        except:
            pass

        exc = []
        try:
            if len(self.exclude_sets) > 0:
                exc = [int(s.strip()) for s in self.exclude_sets.split(",")]
        except:
            pass

        t = sample.get_trace(self.target_trace)
        i1 = np.all(t.data[:,inc] > 0, axis=1)
        i2 = np.all(t.data[:,exc] == 0, axis=1)
        inds = np.logical_and(i1, i2)
        i1 = i2 = None

        for tname in sample.get_trace_names():
            t = sample.get_trace(tname)
            t.data = t.data[inds,:]
            sample.set_trace(tname, t)
        
        return sample

    def save(self):
        """Return an object representing the filter's internal state.

        The object will be later encoded into JSON."""
        desc = Filter.save(self)
        desc["trace"] = self.target_trace
        desc["include_sets"] = self.include_sets
        desc["exclude_sets"] = self.exclude_sets

        return desc

    def load(self, desc):
        """Take the object and set the sink's internal state.

        Returns a reference to the current object."""
        Filter.load(self, desc)
        self.target_trace = desc["trace"]
        self.include_sets = desc["include_sets"]
        self.exclude_sets = desc["exclude_sets"]
        
        return self
