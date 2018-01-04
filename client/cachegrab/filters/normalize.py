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

class NormalizeFilterGui(FilterGui):
    """GUI for normalize filter"""

    def __init__(self, parent, *args):
        """Initialize"""
        FilterGui.__init__(self, parent, *args)

        thresh_sel = wx.BoxSizer(wx.HORIZONTAL)
        
        txt = wx.StaticText(self, wx.ID_ANY, "Cutoff (%)")
        self.thresh = FS.FloatSpin(
            self,
            wx.ID_ANY,
            min_val=0,
            max_val=100,
            increment=0.1,
            value=self.filt.threshold,
            agwStyle=FS.FS_RIGHT
        )
        self.thresh.SetDigits(1)
        
        thresh_sel.Add(txt, 1, wx.ALIGN_CENTER)
        thresh_sel.Add(self.thresh, 0)

        szr_flags = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
        self.sizer.Add(thresh_sel, 1, szr_flags, 5)

        self.SetSizer(self.sizer)
        self.sizer.SetSizeHints(self)

        # Initialize events
        self.thresh.Bind(FS.EVT_FLOATSPIN, self.onchange_thresh)

    def onchange_thresh(self, evt):
        """The threshold was changed"""
        self.filt.threshold = self.thresh.GetValue()
        

class NormalizeFilter(Filter):
    """Normalize the data in the traces.

    This filter finds the nth percentile value within each set, then subtracts
    that value from the set. This way, if most of the datapoints for a set
    are 1 or greater, it gets normalized to 0 or greater.
    """

    name = "Normalize"
    typename = "Normalize"

    def __init__(self):
        """Initializer"""
        Filter.__init__(self)

        self.threshold = 1.0

    def filter_trace(self, trace):
        """Filter and return a single trace from a Sample"""
        tdata = trace.data
        cutoffs = np.percentile(
            tdata,
            self.threshold,
            axis=0,
            keepdims=True
        )
        cutoffs = cutoffs.astype(tdata.dtype)
        inds = tdata < cutoffs
        tdata -= cutoffs
        tdata[inds] = 0
        return Trace(tdata)
    
    def filter(self, sample):
        """Filter and return a single Sample"""
        for name in sample.get_trace_names():
            t = sample.get_trace(name)
            t = self.filter_trace(t)
            sample.set_trace(name, t)
        return sample

    def save(self):
        """Return an object representing the filter's internal state.

        The object will be later encoded into JSON."""
        desc = Filter.save(self)
        desc["threshold"] = self.threshold
        return desc

    def load(self, desc):
        """Take the object and set the sink's internal state.

        Returns a reference to the current object."""
        Filter.load(self, desc)
        self.threshold = desc["threshold"]
        return self
