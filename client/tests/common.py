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

from .context import cachegrab

import numpy as np

from cachegrab.sources import CaptureSource
from cachegrab.filters import Filter
from cachegrab.sinks import AnalysisSink
from cachegrab.data import Sample, Trace

class TestSource(CaptureSource):
    """Implement basic source of samples."""
    def __init__(self):
        CaptureSource.__init__(self)
        self._ind = 0

    def capture_once(self):
        """Return a Sample from a newly executed capture."""
        tr = Trace(np.array([[self._ind]]))
        samp = Sample()
        samp.add_trace("test", tr)
        self._ind += 1
        return samp

class TestFilter(Filter):
    """Implement a basic filter to invert Sample.val"""
    def filter(self, sample):
        for name in sample.get_trace_names():
            t = sample.get_trace(name)
            t.data *= -1
            sample.set_trace(name, t)
        return sample

class TestSink(AnalysisSink):
    """Implement basic sink for samples."""
    def __init__(self):
        """Initialize analysis state."""
        AnalysisSink.__init__(self)
        self.vals = []
    
    def analyze_once(self, sample):
        """Analyze all samples"""
        self.vals.append(sample.get_trace("test").data[0,0])
