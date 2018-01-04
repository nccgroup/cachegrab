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

import cachegrab
from cachegrab.data import Dataset
from cachegrab import Environment

class DatasetExplorer(wx.Panel):
    """Panel displaying the different Datasets in the environment.

    This panel shows the different datasets of an environment, the
    samples within them, and enables the creation or deletion of
    datasets and samples."""

    def __init__(self, parent, env):
        """Set up GUI and load in data"""
        wx.Panel.__init__(self, parent, -1)

        # Initialize controller
        self.env = env

        # Initialize view
        sizer = wx.BoxSizer(wx.VERTICAL)

        lbl = wx.StaticText(self, wx.ID_ANY, "Datasets", style=wx.ALIGN_LEFT)

        self.datasets = wx.ListBox(self, wx.ID_ANY)

        self.btn_add = wx.Button(self, wx.ID_ANY, "Add")
        self.btn_mv = wx.Button(self, wx.ID_ANY, "Rename")
        self.btn_imp = wx.Button(self, wx.ID_ANY, "Import")
        self.btn_exp = wx.Button(self, wx.ID_ANY, "Export")
        self.btn_del = wx.Button(self, wx.ID_ANY, "Delete")
        self.btn_act = wx.Button(self, wx.ID_ANY, "Activate")

        grid = wx.GridSizer(3, 2)
        grid.AddMany([
            (self.btn_add, 0, wx.EXPAND),
            (self.btn_mv, 0, wx.EXPAND),
            (self.btn_imp, 0, wx.EXPAND),
            (self.btn_exp, 0, wx.EXPAND),
            (self.btn_del, 0, wx.EXPAND),
            (self.btn_act, 0, wx.EXPAND),
        ])
        
        sep = wx.StaticLine(self, wx.ID_ANY)

        self.samples_lbl = wx.StaticText(self,
                                         wx.ID_ANY,
                                         "",
                                         style=wx.ALIGN_LEFT)
        self.samples = wx.ListCtrl(self, wx.ID_ANY, style=wx.LC_REPORT)
        self.samples.InsertColumn(0, "ID")

        sizer.Add(lbl, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 10)
        sizer.Add(self.datasets, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 10)
        sizer.Add(grid, 0, wx.EXPAND | wx.ALL, 10)
        sizer.Add(sep, 0, wx.EXPAND)
        sizer.Add(self.samples_lbl,
                  0,
                  wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP,
                  10)
        sizer.Add(self.samples, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 10)
        sizer.SetSizeHints(self)
        self.SetSizer(sizer)

        # Initialize events
        pub.subscribe(self.display_samples, Dataset.SAMPLE_ADDED)
        pub.subscribe(self.display_samples, Environment.DATASET_ACTIVATED)

        self.Bind(wx.EVT_BUTTON, self.add_dataset, self.btn_add)
        self.Bind(wx.EVT_LISTBOX, self.onclick_dataset, self.datasets)
        self.Bind(wx.EVT_BUTTON, self.onclick_import, self.btn_imp)
        self.Bind(wx.EVT_BUTTON, self.onclick_export, self.btn_exp)
        self.Bind(wx.EVT_BUTTON, self.rename_dataset, self.btn_mv)
        self.Bind(wx.EVT_BUTTON, self.rm_dataset, self.btn_del)
        self.Bind(wx.EVT_BUTTON, self.act_dataset, self.btn_act)
        self.Bind(wx.EVT_LISTBOX_DCLICK, self.act_dataset, self.datasets)
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED,
                  self.ondblclick_samples,
                  self.samples)

        # Draw data
        self.refresh_datasets()

    def add_dataset (self, evt):
        """Add a new dataset to the model"""
        dlg = wx.TextEntryDialog(
            self,
            "Enter the name of the new dataset.",
            "Add Dataset")

        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            name = self.env.add_dataset(Dataset(), name)
            self.datasets.AppendAndEnsureVisible(name)
            cnt = self.env.get_dataset_count()
            self.datasets.Select(cnt - 1)
            self.display_samples()
            self.set_button_states()
        dlg.Destroy()

    def rename_dataset (self, evt):
        """Rename the datset."""
        ind = self.datasets.GetSelection()
        if ind < 0:
            return

        name, _ = self.env.get_dataset_by_ind(ind)
        dlg = wx.TextEntryDialog(
            self,
            "Enter the new name for the %s dataset." % name,
            "Add Dataset",
            name
        )

        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            old_name = self.datasets.GetString(ind)
            name = self.env.rename_dataset(old_name, name)
            self.refresh_datasets()
            self.datasets.Select(ind)
            self.set_button_states()
        dlg.Destroy()

        return

    def rm_dataset(self, evt):
        """Remove a dataset from the model"""
        ind = self.datasets.GetSelection()
        if ind >= 0:
            name = self.datasets.GetString(ind)
            self.env.remove_dataset(name)
            self.refresh_datasets()

    def act_dataset(self, evt):
        """Callback when user pressed a button to activate a dataset.

        Activating a dataset means that dataset will be used to accept
        incoming samples from the source, and will be used to provide samples
        to the analysis sink."""
        ind = self.datasets.GetSelection()
        if ind >= 0:
            name = self.datasets.GetString(ind)
            self.env.activate_dataset(name)

    def onclick_import(self, evt):
        """The import button was clicked."""
        # Show directory picker
        dlg = wx.DirDialog(self, "Choose a directory:")
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            try:
                ds = Dataset().load(path)
            except IOError as e:
                err = wx.MessageDialog(
                    self,
                    'Failed to import dataset from %s.' % path,
                    'Import Error',
                    wx.OK
                )
                err.ShowModal()
                err.Destroy()
            else:
                dlg2 = wx.TextEntryDialog(
                    self,
                    "Enter the name of the imported dataset.",
                    "Import Dataset")
                if dlg2.ShowModal() == wx.ID_OK:
                    name = dlg2.GetValue()
                    name = self.env.add_dataset(ds, name)
                    self.datasets.AppendAndEnsureVisible(name)
                    cnt = self.env.get_dataset_count()
                    self.datasets.Select(cnt - 1)
                    self.display_samples()
                    self.set_button_states()
                dlg2.Destroy()
        dlg.Destroy()

    def onclick_export(self, evt):
        """The export button was clicked."""
        ind = self.datasets.GetSelection()
        if ind < 0:
            return

        name = self.datasets.GetString(ind)
        ds = self.env.get_dataset_by_name(name)

        # Show directory picker
        dlg = wx.DirDialog(self, "Choose a directory to save %s:" % name)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            try:
                ds.save(path)
            except IOError as e:
                err = wx.MessageDialog(
                    self,
                    'Failed to export dataset to %s.' % path,
                    'Export Error',
                    wx.OK
                )
                err.ShowModal()
                err.Destroy()
            else:
                msg = wx.MessageDialog(
                    self,
                    'Successfully exported dataset to %s.' % path,
                    'Export Success',
                    wx.OK
                )
                msg.ShowModal()
                msg.Destroy()
            
        dlg.Destroy()
        
    def onclick_dataset(self, evt):
        """An element in the dataset list was clicked"""
        self.set_button_states()
        self.display_samples()

    def ondblclick_samples(self, evt):
        """One of the samples was doubleclicked."""
        ind = self.samples.GetFocusedItem()
        name = self.env.get_active_dataset_name()
        cnt = len(self.env.get_dataset_by_name(name))
        if ind == cnt - 1:
            self.env.set_preview_sample(Environment.AUTO_PREVIEW)
        elif ind >= 0:
            self.env.set_preview_sample(ind)
    
    def refresh_datasets(self):
        """Retrieve the datasets from the model and draw to screen."""
        cnt = self.env.get_dataset_count()
        dataset_names = []
        for i in range(cnt):
            k, v = self.env.get_dataset_by_ind(i)
            dataset_names.append(k)

        self.datasets.Set(dataset_names)
        self.set_button_states()
        self.display_samples()

    def item_selected(self):
        """An item has been selected."""
        self.btn_mv.Enable()
        self.btn_exp.Enable()
        self.btn_del.Enable()
        self.btn_act.Enable()

    def item_deselected(self):
        """No item is selected."""
        self.btn_mv.Disable()
        self.btn_exp.Disable()
        self.btn_del.Disable()
        self.btn_act.Disable()

    def set_button_states(self):
        """If a dataset is not selected, disable buttons"""
        ind = self.datasets.GetSelection()
        if (ind >= 0):
            self.item_selected()
        else:
            self.item_deselected()

    def display_samples(self):
        """Show the samples of the activated dataset in the ListCtrl"""
        name = self.env.get_active_dataset_name()

        ind = self.datasets.GetSelection()
        if ind >= 0:
            sel_name = self.datasets.GetString(ind)
        else:
            sel_name = None

        self.samples.DeleteAllItems()
        if name == None:
            self.samples_lbl.SetLabel("No dataset is active.")
            self.samples.Hide()
        elif sel_name is not None and name != sel_name:
            lbl_text = "%s is not the active dataset." % sel_name
            self.samples_lbl.SetLabel(lbl_text)
            self.samples.Hide()
        else:
            ds = self.env.get_dataset_by_name(name)
            for i, sample in zip(range(len(ds)), ds):
                self.samples.Append([i])
            self.samples_lbl.SetLabel("%s samples:" % name)
            self.samples_lbl.Show()
            self.samples.Show()
        self.Layout()
