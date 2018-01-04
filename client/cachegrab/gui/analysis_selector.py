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

from pubsub import pub
import wx
from wx.lib.scrolledpanel import ScrolledPanel

import cachegrab
from cachegrab import Environment, Pipeline

class AnalysisSettings(wx.Panel):
    """Panel displaying analysis settings.

    Includes source dataset and whether or not to preview analysis."""
    
    # Default dataset label
    DEFAULT_DS_LABEL = "No dataset selected. Select one in Datasets tab."

    def __init__(self, parent, env):
        """Set up GUI and load in data"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SIMPLE)

        # Initialize controller
        self.env = env
        self.pipeline = self.env.analysis_pipeline

        # Initialize view
        sizer = wx.BoxSizer(wx.VERTICAL)
        szr_flags = wx.EXPAND | wx.LEFT | wx.BOTTOM | wx.RIGHT
        filt_row = wx.BoxSizer(wx.HORIZONTAL)

        self.ds_lbl = wx.StaticText(self, wx.ID_ANY, self.DEFAULT_DS_LABEL)
        sizer.Add(self.ds_lbl, 0, wx.EXPAND | wx.ALL, 10)

        self.prev_cb = wx.CheckBox(self, wx.ID_ANY, "Preview Sample Analysis")
        self.Bind(wx.EVT_CHECKBOX, self.preview_clicked, self.prev_cb)

        self.btn_analyze = wx.Button(self, wx.ID_ANY, "Analyze All")
        self.btn_analyze_prev = wx.Button(self, wx.ID_ANY, "Analyze Preview")

        self.btn_add_filter = wx.Button(self, wx.ID_ANY, "Add Filter")
        self.filt_choice = wx.Choice(self, wx.ID_ANY)
        filt_row.Add(self.filt_choice, 1)
        filt_row.Add(self.btn_add_filter, 0)

        sizer.Add(self.prev_cb, 0, szr_flags, 10)
        sizer.Add(self.btn_analyze, 0, szr_flags, 10)
        sizer.Add(self.btn_analyze_prev, 0, szr_flags, 10)
        sizer.Add(filt_row, 0, szr_flags, 10)

        sizer.SetSizeHints(self)
        self.SetSizer(sizer)

        # Initialize events
        self.Bind(wx.EVT_BUTTON, self.analyze_many, self.btn_analyze)
        self.Bind(wx.EVT_BUTTON, self.analyze_prev, self.btn_analyze_prev)
        self.Bind(wx.EVT_BUTTON, self.onclick_add_filter, self.btn_add_filter)

        pub.subscribe(self.draw_active_dataset, Environment.DATASET_ACTIVATED)
        pub.subscribe(self.on_preview_changed, Environment.PREV_SAMP_CHANGED)

        # Fill in data
        self.draw_all()
        self.draw_filter_choices()
        self.on_preview_changed()

    def onclick_add_filter(self, evt):
        """The user wishes to add a filter to the source collection."""
        ind = self.filt_choice.GetSelection()
        FilterClass = cachegrab.filters.get_filters()[ind]
        filt = FilterClass()
        self.pipeline.add_filter(filt)

    def draw_filter_choices(self):
        """Draw the different filter choices into the Choice"""
        filters = cachegrab.filters.get_filters()
        if len(filters) == 0:
            self.btn_add_filter.Disable()
        else:
            names = [cls.name for cls in filters]
            self.filt_choice.SetItems(names)
            self.filt_choice.Select(0)

    def draw_active_dataset(self):
        """Callback for when the active dataset is changed."""
        name = self.env.get_active_dataset_name()
        if name is not None:
            lbl = "Samples are coming from the %s dataset." % name
            self.ds_lbl.SetLabel(lbl)
            self.Layout()
        else:
            lbl = self.DEFAULT_DS_LABEL
            self.ds_lbl.SetLabel(lbl)
            self.Layout()

    def draw_checkbox(self):
        """Update the state of the checkbox"""
        val = self.env.preview_analysis
        self.prev_cb.SetValue(val)

    def draw_all(self):
        """Update all elements in ppview"""
        self.draw_active_dataset()
        self.draw_checkbox()

    def preview_clicked(self, evt):
        """Callback for when the checkbox for analysis preview is clicked."""
        val = self.prev_cb.GetValue()
        self.env.set_preview_analysis(val)
            
    def analyze_many(self, evt):
        """Callback for when all samples need to be analyzed"""
        # TODO: Background thread
        self.pipeline.run()

    def analyze_prev(self, evt):
        """Callback for when the preview needs to be reanalyzed."""
        self.env.preview_changed()

    def on_preview_changed(self):
        """Callback for when the preview has changed"""
        samp = self.env.get_preview()
        self.btn_analyze_prev.Enable(samp is not None)

class AnalysisSelector(ScrolledPanel):
    """Panel displaying the settings for analyzing samples.

    This panel shows the analysis settings, the analysis module to use,
    and whether or not to preview analysis."""

    def __init__(self, parent, env):
        """Set up GUI and load in data"""

        ScrolledPanel.__init__(self, parent, -1)

        # Initialize controller
        self.env = env
        self.pipeline = self.env.analysis_pipeline

        # Initialize view
        self.sizer = wx.BoxSizer(wx.VERTICAL)

        settings = AnalysisSettings(self, self.env)
        self.sizer.Add(settings, 0, wx.EXPAND | wx.ALL, 10)

        self.sizer.SetSizeHints(self)
        self.SetSizer(self.sizer)
        self.SetAutoLayout(True)
        self.SetupScrolling()

        # Pubsub events
        pub.subscribe(self.on_filters_changed, Pipeline.FILTERS_CHANGED)

        # Draw initial data
        self.draw_filters()

    def add_filter(self, filt):
        """Add the given filter from the model to the GUI"""
        def delete_func(evt, filt=filt):
            self.pipeline.remove_filter(filt)

        def up_func(evt, filt=filt):
            self.pipeline.move_filter(filt, -1)

        def down_func(evt, filt=filt):
            self.pipeline.move_filter(filt, 1)

        GuiCls = cachegrab.filters.get_filter_gui(filt)
        filt_view = GuiCls(self, filt, delete_func, up_func, down_func)
        self.sizer.Add(
            filt_view,
            0,
            wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
            10
        )

    def clear_filters(self):
        """Remove all filters from the GUI"""
        while self.sizer.GetItemCount() - 1 > 0:
            # Delete all filters after collection settings
            self.sizer.Hide(1)
            self.sizer.Remove(1)

    def draw_filters(self):
        """Draw all filters to the view."""
        self.clear_filters()
        filts = self.pipeline.get_filters()
        for filt in filts:
            self.add_filter(filt)
        self.SetupScrolling()

    def on_filters_changed(self, pipeline):
        """Callback when the filters change"""
        if pipeline == self.pipeline:
            self.draw_filters()        
