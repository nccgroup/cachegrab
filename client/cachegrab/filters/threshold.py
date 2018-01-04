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

class ThresholdFilterGui(FilterGui):
    """GUI for threshold filter"""

    def __init__(self, parent, *args):
        """Initialize"""
        FilterGui.__init__(self, parent, *args)

        self.trace = wx.TextCtrl(self, wx.ID_ANY)
        self._add_field("Trace to Filter", self.trace, small=True)

        self.thresh = FS.FloatSpin(
            self,
            wx.ID_ANY,
            min_val=0,
            max_val=100,
            increment=0.1,
            value=0,
            agwStyle=FS.FS_RIGHT
        )
        self.thresh.SetDigits(1)
        self._add_field("Cutoff (%)", self.thresh, small=True)

        self.keep_below = wx.CheckBox(self, wx.ID_ANY)
        self._add_field("Keep samples below threshold?",
                        self.keep_below,
                        small=True)

        self.SetSizer(self.sizer)
        self.sizer.SetSizeHints(self)

        # Initialize events
        self.thresh.Bind(FS.EVT_FLOATSPIN, self.onchange_thresh)
        self.trace.Bind(wx.EVT_TEXT, self.onchange_trace)
        self.keep_below.Bind(wx.EVT_CHECKBOX, self.oncheck_kb)

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

    def onchange_thresh(self, evt):
        """The threshold was changed"""
        self.filt.threshold = self.thresh.GetValue()

    def onchange_trace(self, evt):
        """The trace was changed"""
        self.filt.target_trace = self.trace.GetValue()

    def oncheck_kb(self, evt):
        """The keep_below checkbox was clicked"""
        kb = self.keep_below.GetValue()
        self.filt.keep_below = kb

    def draw(self):
        """Draw all data"""
        self.thresh.SetValue(self.filt.threshold)
        self.trace.ChangeValue(self.filt.target_trace)
        self.keep_below.SetValue(self.filt.keep_below)

class ThresholdFilter(Filter):
    """Remove samples from the trace that exceed a certain density.

    This filter focuses on a single trace and finds time indices that
    have densities exceeding a certain value. It then removes those
    time slices from all filters.
    """

    name = "Threshold"
    typename = "Threshold"

    def __init__(self):
        """Initializer"""
        Filter.__init__(self)

        self.threshold = 50.0
        self.target_trace = ""
        self.keep_below = True

    def filter(self, sample):
        """Filter and return a single Sample"""
        if self.target_trace not in sample.get_trace_names():
            return sample

        t = sample.get_trace(self.target_trace)
        sums = np.sum(t.data > 0, axis=1)
        cutoff = self.threshold * t.data.shape[1] / 100.

        if self.keep_below:
            inds = sums <= cutoff
            sums = None
            cutoff = None
        else:
            inds = sums > cutoff
            sums = None
            cutoff = None

        for tname in sample.get_trace_names():
            t = sample.get_trace(tname)
            t.data = t.data[inds,:]
            sample.set_trace(tname, t)
            
        return sample

    def save(self):
        """Return an object representing the filter's internal state.

        The object will be later encoded into JSON."""
        desc = Filter.save(self)
        desc["threshold"] = self.threshold
        desc["trace"] = self.target_trace
        desc["keep_below"] = "y" if self.keep_below else "n"
        return desc

    def load(self, desc):
        """Take the object and set the sink's internal state.

        Returns a reference to the current object."""
        Filter.load(self, desc)
        self.threshold = desc["threshold"]
        self.target_trace = desc["trace"]
        self.keep_below = desc["keep_below"] == "y"
        return self
