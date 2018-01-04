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
from pubsub import pub

from cachegrab.sinks import AnalysisSink

class AnalysisSimpleGui(wx.Panel):
    """View corresponding to AnalysisSimple sink"""

    def __init__(self, parent, simple_sink):
        """Set up GUI and load in data"""
        wx.Panel.__init__(self, parent, wx.ID_ANY)

        # Initialize controller
        self.simple = simple_sink

        # Initialize view
        sizer = wx.BoxSizer(wx.VERTICAL)

        self.log = wx.TextCtrl(self, wx.ID_ANY, style=wx.TE_MULTILINE)
        sizer.Add(self.log, 1, wx.EXPAND)

        sizer.SetSizeHints(self)
        self.SetSizer(sizer)

        # Register to pubsub events
        pub.subscribe(self.log_changed, AnalysisSimple.LOG_ADDED)

        # Fill in views

    def log_changed(self, log):
        self.log.write(log)


class AnalysisSimple(AnalysisSink):
    """Logs basic information about the collected sample."""

    typename = "Simple_Analysis"

    # Different pubsub events
    LOG_ADDED = "ANALYSIS_SIMPLE_LOG_ADDED"

    def __init__(self):
        """Initialize the sink object."""
        AnalysisSink.__init__(self)
        self.loglines = ""

    def log(self, val):
        """Write the specified value to the loglines, followed by newline."""
        with_newline = val + "\n"
        self.loglines += with_newline
        pub.sendMessage(AnalysisSimple.LOG_ADDED, log=with_newline)

    def analyze_once(self, sample):
        """Analyze a single sample."""
        trace_names = sample.get_trace_names()
        logline = "Traces: " + " ".join(trace_names)
        trace = sample.get_trace(trace_names[0]).data
        logline += " shape: " + str(trace.shape)
        self.log(logline)

    def get_log(self):
        """Return the log text"""
        return self.loglines
