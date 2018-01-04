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

import base64
from matplotlib.figure import Figure
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg
from matplotlib.backends.backend_wxagg import NavigationToolbar2WxAgg
import matplotlib.pyplot as plt
from pubsub import pub
import wx
import wx.lib.scrolledpanel as scrolled

import cachegrab
from cachegrab.sinks import AnalysisSink

class Navigation(NavigationToolbar2WxAgg):
    """Subclass of MPL Navigation toolbar.

    Created to add custom cursor over pan/zoom"""

    def __init__(self, canvas, axes):
        """Initialize Navigation"""
        NavigationToolbar2WxAgg.__init__(self, canvas)
        self.pan()

        self.canvas = canvas
        self.axes = axes
        self.setup_lines()

    def setup_lines(self):
        """Sets up the lines for all plots"""
        self.lines = []
        for ax in self.axes:
            xline = ax.axvline(0, visible=False)
            yline = ax.axhline(0, visible=False)
            self.lines.append((xline, yline))

    def draw_lines(self, ax, x, y):
        """Draw the line in all the subplots

        The x coordinate is shared, and the y coordinate is only
        used for the current axes."""
        x = int(round(x))
        y = int(round(y))
        for i in range(len(self.axes)):
            xline, yline = self.lines[i]
            xline.set_xdata(x)
            xline.set_visible(True)
            if ax is self.axes[i]:
                yline.set_ydata(y)
                yline.set_visible(True)
            else:
                yline.set_visible(False)
        self.canvas.draw_idle()

    def clear_lines(self):
        """Remove all lines from all plots"""
        for xline, yline in self.lines:
            xline.set_visible(False)
            yline.set_visible(False)
        self.canvas.draw_idle()

    def mouse_move(self, event):
        """Draw lines when mouseover the axes"""
        if event.inaxes is not None:
            x, y = event.xdata, event.ydata
            self.draw_lines(event.inaxes, x, y)
        else:
            self.clear_lines()

        return NavigationToolbar2WxAgg.mouse_move(self, event)

class AnalysisGraphGui(wx.Panel):
    """View corresponding to AnalysisGraph sink"""

    def __init__(self, parent, sink):
        """Set up GUI and load in data"""
        wx.Panel.__init__(self, parent, wx.ID_ANY)

        # Initialize controller
        self.graph = sink
        self.figure = None

        # Initialize view
        # Use a sizer so we can dynamically remove and recreate the graph
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.SetSizeHints(self)
        self.SetSizer(self.sizer)

        self.extra = wx.BoxSizer(wx.HORIZONTAL)

        stdout = wx.BoxSizer(wx.VERTICAL)
        stderr = wx.BoxSizer(wx.VERTICAL)

        out_lbl = wx.StaticText(self, wx.ID_ANY, "STDOUT")
        self.stdout_txt = wx.TextCtrl(self, wx.ID_ANY, "",
                                      style=wx.TE_READONLY
                                      | wx.TE_MULTILINE
                                      | wx.TE_DONTWRAP)
        stdout.Add(out_lbl, 0, wx.ALIGN_CENTER)
        stdout.Add(self.stdout_txt, 1, wx.EXPAND)

        err_lbl = wx.StaticText(self, wx.ID_ANY, "STDERR")
        self.stderr_txt = wx.TextCtrl(self, wx.ID_ANY, "",
                                      style=wx.TE_READONLY
                                      | wx.TE_MULTILINE
                                      | wx.TE_DONTWRAP)
        stderr.Add(err_lbl, 0, wx.ALIGN_CENTER)
        stderr.Add(self.stderr_txt, 1, wx.EXPAND)

        self.extra.Add(stdout, 1, wx.EXPAND | wx.ALIGN_CENTER)
        self.extra.Add(stderr, 1, wx.EXPAND | wx.ALIGN_CENTER)
        
        self.sizer.Add(self.extra, 1, wx.EXPAND)
        self.sizer.Hide(0)

        # Register to pubsub events
        pub.subscribe(self.sample_changed, AnalysisGraph.SAMPLE_ADDED)
        pub.subscribe(self.clean_traces, AnalysisGraph.SAMPLE_RESET)

    def draw_subplot(self, ax, name, data):
        title = "Probe %s" % name
        ax.set_title(title)
        if data.shape[0] == 0:
            return
        
        ax.imshow(
            data.T,
            cmap="gray",
            aspect="auto",
            interpolation="None"
        )

    def draw_sample(self, sample):
        """Add the matplotlib graph to the sizer."""
        self.clean_traces()

        self.sizer.Show(0)

        sout = base64.b64decode(sample.get_extra("stdout"))
        serr = base64.b64decode(sample.get_extra("stderr"))
        self.stdout_txt.SetValue(sout)
        self.stderr_txt.SetValue(serr)
        
        names = sample.get_trace_names()
        cnt = len(names)
        if cnt == 0:
            lbl_text = "The provided sample has no traces"
            lbl = wx.StaticText(self, wx.ID_ANY, lbl_text)
            self.sizer.Insert(0, lbl, 3, wx.ALIGN_CENTER)
            self.Layout()
            return
        elif cnt == 1:
            self.figure, ax = plt.subplots(cnt, 1, sharex=True)
            axes = [ax]
        else:
            self.figure, axes = plt.subplots(cnt, 1, sharex=True)
        
        canvas = FigureCanvasWxAgg(self, wx.ID_ANY, self.figure)
        nav = Navigation(canvas, axes)

        for i in range(cnt):
            ax = axes[i]
            name = names[i]
            data = sample.get_trace(name).data
            self.draw_subplot(ax, name, data)
        self.figure.tight_layout()
        axes[0].set_xlim(0, 1000)
        
        # Hide navigation toolbar
        #nav.Realize()
        #self.sizer.Add(nav, 0, wx.EXPAND)
        self.sizer.Insert(0, canvas, 3, wx.EXPAND)
        self.Layout()

    def clean_traces(self):
        """Remove all traces from the view"""
        while self.sizer.GetItemCount() > 1:
            self.sizer.Hide(0)
            self.sizer.Remove(0)
        self.sizer.Hide(0)
        self.Layout()
        if self.figure is not None:
            plt.close(self.figure)
            self.figure = None

    def sample_changed(self, sample):
        self.draw_sample(sample)

class AnalysisGraph(AnalysisSink):
    """Displays the trace as a bitmapped image"""

    typename = "Graph_Analysis"

    # Different pubsub events
    SAMPLE_ADDED = "ANALYSIS_GRAPH_SAMPLE_ADDED"
    SAMPLE_RESET = "ANALYSIS_GRAPH_SAMPLE_RESET"

    def __init__(self):
        """Initialize the sink object."""
        AnalysisSink.__init__(self)

    def analyze_once(self, sample):
        """Analyze a single sample."""
        # Nothing to do but notify the view of the data
        pub.sendMessage(AnalysisGraph.SAMPLE_ADDED, sample=sample)

    def clean(self):
        """Resets the analysis to the default state."""
        pub.sendMessage(AnalysisGraph.SAMPLE_RESET)
