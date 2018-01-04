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
from cachegrab.sources import CaptureSource

class CollectionSettings(wx.Panel):
    """Panel displaying collection settings like dataset and sample count."""

    # Default dataset label
    DEFAULT_DS_LABEL = "No dataset selected. Select one in Datasets tab."

    def __init__(self, parent, env):
        """Set up GUI and load in data"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SIMPLE)

        # Initialize controller
        self.env = env
        self.pipeline = self.env.collection_pipeline

        # Initialize view
        sizer = wx.BoxSizer(wx.VERTICAL)
        filt_row = wx.BoxSizer(wx.HORIZONTAL)

        self.ds_lbl = wx.StaticText(self, wx.ID_ANY, self.DEFAULT_DS_LABEL)

        self.btn_collect = wx.Button(self, wx.ID_ANY, "Collect Once")

        self.btn_add_filter = wx.Button(self, wx.ID_ANY, "Add Filter")
        self.filt_choice = wx.Choice(self, wx.ID_ANY)
        filt_row.Add(self.filt_choice, 1)
        filt_row.Add(self.btn_add_filter, 0)

        sizer.Add(self.ds_lbl, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 10)
        sizer.Add(self.btn_collect,
                  0,
                  wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 10)
        sizer.Add(filt_row, 0, wx.EXPAND | wx.ALL, 10)

        sizer.SetSizeHints(self)
        self.SetSizer(sizer)

        # Initialize events
        self.Bind(wx.EVT_BUTTON, self.collect_once, self.btn_collect)
        self.Bind(wx.EVT_BUTTON, self.onclick_add_filter, self.btn_add_filter)

        pub.subscribe(self.update_dataset, Environment.DATASET_ACTIVATED)
        pub.subscribe(self.update_source, CaptureSource.READINESS_CHANGED)

        # Fill in data
        self.update_dataset()
        self.draw_filter_choices()

    def collect_once (self, evt):
        """Collect one sample to the active dataset"""
        if self.env.get_active_dataset_name() is not None:
            self.env.collection_pipeline.run(1)

    def onclick_add_filter(self, evt):
        """The user wishes to add a filter to the source collection."""
        ind = self.filt_choice.GetSelection()
        FilterClass = cachegrab.filters.get_filters()[ind]
        filt = FilterClass()
        self.pipeline.add_filter(filt)

    def update_dataset(self):
        """Callback for when the active dataset is changed."""
        self.draw_button()
        self.draw_dataset_label()

    def update_source(self):
        """Callback for when the source changes."""
        self.draw_button()

    def draw_filter_choices(self):
        """Draw the different filter choices into the Choice"""
        filters = cachegrab.filters.get_filters()
        if len(filters) == 0:
            self.btn_add_filter.Disable()
        else:
            names = [cls.name for cls in filters]
            self.filt_choice.SetItems(names)
            self.filt_choice.Select(0)

    def draw_dataset_label(self):
        """Draw the label with dataset information."""
        name = self.env.get_active_dataset_name()
        if name is not None:
            lbl = "Samples are going to the %s dataset." % name
        else:
            lbl = self.DEFAULT_DS_LABEL
        self.ds_lbl.SetLabel(lbl)
        self.Layout()

    def draw_button(self):
        """Enable or disable the button as appropriate."""
        name = self.env.get_active_dataset_name()
        dataset_valid = name is not None

        src = self.env.scope_source
        source_valid = src.is_ready()

        if dataset_valid and source_valid:
            self.btn_collect.Enable()
        else:
            self.btn_collect.Disable()
        self.Layout()

class SourceCollector(ScrolledPanel):
    """Panel displaying collection settings and collection filters.

    This panel shows the current scope, the number of samples to collect,
    the filters to apply, and the dataset to write to."""

    def __init__(self, parent, env):
        """Set up GUI and load in data"""
        ScrolledPanel.__init__(self, parent, -1)

        # Initialize controller
        self.env = env
        self.pipeline = self.env.collection_pipeline

        # Initialize view
        self.sizer = wx.BoxSizer(wx.VERTICAL)

        settings = CollectionSettings(self, self.env)
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
