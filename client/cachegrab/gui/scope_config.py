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
from wx.lib.intctrl import IntCtrl
from wx.lib.scrolledpanel import ScrolledPanel

from cachegrab.sources import ScopeSource, CaptureSource
from cachegrab.sources.scope_source import Probe

class ScopeConnection(wx.Panel):
    """This is the view and controller for connecting to the scope"""

    def __init__(self, parent, env):
        """Initialize the panel for setting up the scope"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SIMPLE)

        self.env = env
        self.scope = self.env.scope_source

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(self.sizer)
        
        input_host = wx.BoxSizer(wx.HORIZONTAL)
        target_cpu = wx.BoxSizer(wx.HORIZONTAL)
        scope_cpu = wx.BoxSizer(wx.HORIZONTAL)
        buttons = wx.BoxSizer(wx.HORIZONTAL)

        self.status = wx.StaticText(self, wx.ID_ANY, "")
        font = self.status.GetFont()
        font.SetWeight(wx.BOLD)
        self.status.SetFont(font)

        self.btn_enable = wx.Button(self, wx.ID_ANY, "")
        self.btn_collect = wx.Button(self, wx.ID_ANY, "Collect")

        buttons.Add(self.btn_enable, 1, wx.ALIGN_CENTER)
        buttons.Add(self.btn_collect, 1, wx.ALIGN_CENTER)

        lbl_server = wx.StaticText(self, wx.ID_ANY, "Scope Server")
        self.txt_server = wx.TextCtrl(self, wx.ID_ANY, self.scope.server)
        self.btn_connect = wx.Button(self, wx.ID_ANY, "Connect")
        input_host.Add(lbl_server, 0, wx.ALIGN_CENTER | wx.RIGHT, 5)
        input_host.Add(self.txt_server, 1, wx.EXPAND)
        input_host.Add(self.btn_connect, 0, wx.EXPAND)

        self.sizer.Add(self.status, 0, wx.EXPAND | wx.ALL, 5)
        self.sizer.Add(input_host, 0,
                       wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT, 5)
        
        self.choice_t_cpu = wx.Choice(self, wx.ID_ANY)
        self._add_field("Target CPU", self.choice_t_cpu, small=True)
        
        self.choice_s_cpu = wx.Choice(self, wx.ID_ANY)
        self._add_field("Scope CPU", self.choice_s_cpu, small=True)

        self.command = wx.TextCtrl(self, wx.ID_ANY, style=wx.TE_RIGHT)
        self._add_field("Target Command", self.command)
        self.command.Bind(wx.EVT_TEXT, self.onchange_capture_settings)

        self.ta = wx.TextCtrl(self, wx.ID_ANY, style=wx.TE_RIGHT)
        self._add_field("Target App", self.ta)
        self.ta.Bind(wx.EVT_TEXT, self.onchange_capture_settings)

        self.target_cbuf = wx.TextCtrl(self, wx.ID_ANY, style=wx.TE_RIGHT)
        self._add_field("Target Trigger Buffer", self.target_cbuf)
        self.target_cbuf.Bind(wx.EVT_TEXT, self.onchange_capture_settings)

        self.msamples = IntCtrl(self, wx.ID_ANY, style=wx.TE_RIGHT)
        self._add_field("Max # Samples", self.msamples, small=True)
        self.msamples.Bind(wx.EVT_TEXT, self.onchange_capture_settings)

        self.delta = IntCtrl(self, wx.ID_ANY, style=wx.TE_RIGHT)
        self._add_field("Time Step", self.delta, small=True)
        self.delta.Bind(wx.EVT_TEXT, self.onchange_capture_settings)

        self.debug = wx.CheckBox(self, wx.ID_ANY)
        self._add_field("Debug TA Calls", self.debug, small=True)
        self.debug.Bind(wx.EVT_CHECKBOX, self.onchange_capture_settings)
        
        self.sizer.Add(buttons, 0, wx.ALL | wx.CENTER, 5)

        # Events
        self.btn_enable.Bind(wx.EVT_BUTTON, self.onclick_btn_enable)
        self.btn_connect.Bind(wx.EVT_BUTTON, self.onclick_btn_connect)
        self.choice_t_cpu.Bind(wx.EVT_CHOICE, self.onchoice_t_cpu)
        self.choice_s_cpu.Bind(wx.EVT_CHOICE, self.onchoice_s_cpu)
        self.btn_collect.Bind(wx.EVT_BUTTON, self.onclick_btn_collect)

        pub.subscribe(self.on_enable_changed, ScopeSource.ENABLED_CHANGED)
        pub.subscribe(self.on_connect_changed, ScopeSource.CONNECTED_CHANGED)
        pub.subscribe(self.on_capture_settings_changed,
                      ScopeSource.CAPTURE_SETTINGS_CHANGED)
        pub.subscribe(self.on_readiness_changed,
                      CaptureSource.READINESS_CHANGED)


        # Initial data
        self.draw_status()
        self.draw_capture_settings()
        self.draw_collect_btn()

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
        
    def onclick_btn_connect(self, evt):
        """Connect button has been clicked."""
        server = self.txt_server.GetValue()
        if self.scope.is_connected():
            self.scope.disconnect()
        self.scope.connect(server)

    def onclick_btn_enable(self, evt):
        """Enable button has been clicked."""
        if not self.scope.is_enabled():
            self.scope.enable()
        else:
            self.scope.disable()

    def onclick_btn_collect(self, evt):
        """Collect button has been clicked."""
        if self.env.get_active_dataset_name() is not None:
            self.env.collection_pipeline.run(1)

    def onchoice_t_cpu(self, evt):
        """The target CPU has changed."""
        sel = self.choice_t_cpu.GetSelection()
        if sel != wx.NOT_FOUND:
            self.scope.set_target_core(sel)

    def onchoice_s_cpu(self, evt):
        """The scope CPU has changed."""
        sel = self.choice_s_cpu.GetSelection()
        if sel != wx.NOT_FOUND:
            self.scope.set_scope_core(sel)

    def on_enable_changed(self):
        """The enable state of the scope has changed."""
        self.draw_status()
        
    def on_connect_changed(self):
        """The connection state of the scope has changed."""
        self.draw_status()
        self.draw_capture_settings()

    def on_capture_settings_changed(self):
        """The capture settings have changed."""
        self.draw_capture_settings()

    def on_readiness_changed(self):
        """The readiness of the scope has changed."""
        self.draw_collect_btn()

    def onchange_capture_settings(self, evt):
        """The user is trying to change the capture settings."""
        cmd = self.command.GetValue()
        ta = self.ta.GetValue()
        cbuf = self.target_cbuf.GetValue()
        msamp = self.msamples.GetValue()
        delta = self.delta.GetValue()
        debug = self.debug.GetValue()

        self.scope.set_capture_params(
            max_samples=msamp, time_delta=delta, ta_command=cmd,
            ta_name=ta, ta_cbuf=cbuf, debug=debug)

    def draw_status(self):
        """Update the state, buttons, and spinners based on current state"""
        enabled = self.scope.is_enabled()
        connected = self.scope.is_connected()

        if enabled and connected:
            scope_state = "Enabled"
        elif connected and not enabled:
            scope_state = "Connected and Disabled"
        else:
            scope_state = "Disconnected"

        btn_label = "Disable" if enabled else "Enable"
        scope_status = "Scope is %s" % scope_state

        self.status.SetLabel(scope_status)
        self.btn_connect.Enable(not enabled or not connected)
        self.txt_server.Enable(not enabled or not connected)
        self.btn_enable.SetLabel(btn_label)
        self.btn_enable.Enable(connected)

        ncores = self.scope.num_cores
        cores = ["Core %u" % i for i in range(ncores)]

        sel = self.choice_t_cpu.GetSelection()
        if sel == wx.NOT_FOUND:
            sel = self.scope.get_target_core()
        self.choice_t_cpu.SetItems(cores)
        if ncores > 0:
            self.choice_t_cpu.SetSelection(sel)
        self.choice_t_cpu.Enable(connected and not enabled)

        sel = self.choice_s_cpu.GetSelection()
        if sel == wx.NOT_FOUND:
            sel = self.scope.get_scope_core()
        self.choice_s_cpu.SetItems(cores)
        if ncores > 0:
            self.choice_s_cpu.SetSelection(sel)
        self.choice_s_cpu.Enable(connected and not enabled)

    def draw_capture_settings(self):
        """Draw the capture settings."""
        enabled = self.scope.is_enabled()
        connected = self.scope.is_connected()
        
        self.command.ChangeValue(self.scope.ta_command)
        self.ta.ChangeValue(self.scope.ta_name)
        self.target_cbuf.ChangeValue(self.scope.ta_cbuf)
        self.msamples.ChangeValue(self.scope.max_samples)
        self.delta.ChangeValue(self.scope.time_delta)
        self.debug.SetValue(self.scope.debug)

        self.command.Enable(connected)
        self.ta.Enable(connected)
        self.target_cbuf.Enable(connected)
        self.msamples.Enable(connected)
        self.delta.Enable(connected)
        self.debug.Enable(connected)

    def draw_collect_btn(self):
        """Enable or disable the collection button"""
        self.btn_collect.Enable(self.scope.is_ready())

class ProbeConfig(wx.Panel):
    """This is the view and controller for a single probe configuration"""

    def __init__(self, parent, probe):
        """Initialization"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SIMPLE)
        self.probe = probe

        sizer = wx.BoxSizer(wx.VERTICAL)

        assoc = wx.BoxSizer(wx.HORIZONTAL)
        nset = wx.BoxSizer(wx.HORIZONTAL)
        line = wx.BoxSizer(wx.HORIZONTAL)
        capt = wx.BoxSizer(wx.HORIZONTAL)
        btns = wx.BoxSizer(wx.HORIZONTAL)

        self.status = wx.StaticText(self, wx.ID_ANY, "Probe is Active")
        font = self.status.GetFont()
        font.SetWeight(wx.BOLD)
        self.status.SetFont(font)
        
        lbl_assoc = wx.StaticText(self, wx.ID_ANY, "Associativity")
        self.int_assoc = IntCtrl(self, wx.ID_ANY)
        self.int_assoc.SetValue(self.probe.associativity)
        assoc.Add(lbl_assoc, 1, wx.ALIGN_CENTER | wx.RIGHT, 5)
        assoc.Add(self.int_assoc, 0, wx.ALIGN_RIGHT)

        lbl_nset = wx.StaticText(self, wx.ID_ANY, "Number of Sets")
        self.int_nset = IntCtrl(self, wx.ID_ANY)
        self.int_nset.SetValue(self.probe.num_sets)
        nset.Add(lbl_nset, 1, wx.ALIGN_CENTER | wx.RIGHT, 5)
        nset.Add(self.int_nset, 0, wx.ALIGN_RIGHT)

        lbl_line = wx.StaticText(self, wx.ID_ANY, "Size of Line")
        self.int_line = IntCtrl(self, wx.ID_ANY)
        self.int_line.SetValue(self.probe.line_size)
        line.Add(lbl_line, 1, wx.ALIGN_CENTER | wx.RIGHT, 5)
        line.Add(self.int_line, 0, wx.ALIGN_RIGHT)

        lbl_capt = wx.StaticText(self, wx.ID_ANY, "Capture Limits:")
        lbl_lp = wx.StaticText(self, wx.ID_ANY, "[")
        self.spn_lo = wx.SpinButton(self, wx.ID_ANY)
        lbl_cm = wx.StaticText(self, wx.ID_ANY, ",")
        self.spn_hi = wx.SpinButton(self, wx.ID_ANY)
        lbl_rp = wx.StaticText(self, wx.ID_ANY, ")")
        self.btn_config = wx.Button(self, wx.ID_ANY, "Configure")
        capt.Add(lbl_capt, 3, wx.ALIGN_CENTER)
        capt.Add(lbl_lp, 0, wx.ALIGN_CENTER)
        capt.Add(self.spn_lo, 1, wx.ALIGN_CENTER)
        capt.Add(lbl_cm, 0, wx.ALIGN_CENTER)
        capt.Add(self.spn_hi, 1, wx.ALIGN_CENTER)
        capt.Add(lbl_rp, 0, wx.ALIGN_CENTER)
        capt.Add(self.btn_config, 0, wx.ALIGN_CENTER)

        self.btn_enable = wx.Button(self, wx.ID_ANY, "Enable")
        btns.Add(self.btn_enable, 0, wx.ALL, 5)

        sizer.Add(self.status, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, 5)
        sizer.Add(assoc, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, 5)
        sizer.Add(nset, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, 5)
        sizer.Add(line, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, 5)
        sizer.Add(capt, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, 5)
        sizer.Add(btns, 0, wx.ALIGN_CENTER | wx.ALL, 5)

        self.SetSizer(sizer)
        sizer.SetSizeHints(self)

        # Events
        self.btn_enable.Bind(wx.EVT_BUTTON, self.onclick_btn_enable)
        self.btn_config.Bind(wx.EVT_BUTTON, self.onclick_btn_config)

        pub.subscribe(self.on_enable_changed, Probe.ENABLED_CHANGED)
        pub.subscribe(self.on_enable_changed, Probe.CONFIG_CHANGED)

        # Initial data
        self.on_enable_changed()


    def set_status(self):
        """Display the status of the probe."""
        name = self.probe.name
        status = "Probe %s is active" % name
        self.status.SetLabel(status)

    def onclick_btn_enable(self, evt):
        """Enable button has been clicked."""
        if not self.probe.is_enabled():
            assoc = self.int_assoc.GetValue()
            nset = self.int_nset.GetValue()
            lsize = self.int_line.GetValue()
            self.probe.enable(assoc, nset, lsize)
        else:
            self.probe.disable()

    def onclick_btn_config(self, evt):
        """Configure button has been clicked."""
        lo = self.spn_lo.GetValue()
        hi = self.spn_hi.GetValue()
        if not self.probe.configure(lo, hi):
            msg = wx.MessageDialog(
                self,
                'Failed to configure probe %s' % self.probe.name,
                'Failure',
                wx.OK
            )
            msg.ShowModal()
            msg.Destroy()

    def on_enable_changed(self, name=None):
        """The enable state of the scope has changed."""
        if name is not None and self.probe.name != name:
            return

        enabled = self.probe.is_enabled()
        prb_state = "Enabled" if enabled else "Disabled"
        btn_status = "Disable" if enabled else "Enable"
        prb_status = "Probe %s is %s" % (self.probe.name, prb_state)
        self.status.SetLabel(prb_status)
        self.btn_enable.SetLabel(btn_status)

        self.int_assoc.Enable(not enabled)
        self.int_nset.Enable(not enabled)
        self.int_line.Enable(not enabled)
        self.spn_lo.Enable(enabled)
        self.spn_hi.Enable(enabled)
        self.btn_config.Enable(enabled)
        self.spn_lo.SetRange(0, self.probe.num_sets)
        self.spn_hi.SetRange(0, self.probe.num_sets)
        self.spn_lo.SetValue(self.probe.low_set)
        self.spn_hi.SetValue(self.probe.high_set)

class ScopeConfig(ScrolledPanel):
    """This is the view and controller for the scope configuration"""

    def __init__(self, parent, env):
        """Initialization"""
        ScrolledPanel.__init__(self, parent, wx.ID_ANY)

        self.env = env
        self.scope = self.env.scope_source
        
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(self.sizer)
        self.sizer.SetSizeHints(self)
        self.SetAutoLayout(True)
        self.SetupScrolling()

        setup = ScopeConnection(self, self.env)
        self.sizer.Add(setup, 0, wx.EXPAND | wx.ALL, 10)

        # Events
        pub.subscribe(self.draw_probes, ScopeSource.PROBES_CHANGED)

        # Draw initial state
        self.draw_probes()

    def add_probe(self, probe):
        """Add the given probe from the model to the GUI"""
        probe_view = ProbeConfig(self, probe)
        self.sizer.Add(
            probe_view,
            0,
            wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
            10
        )

    def clear_probes(self):
        """Remove all probes from the GUI"""
        while self.sizer.GetItemCount() - 1 > 0:
            # Delete all probes after setup panel
            self.sizer.Hide(1)
            self.sizer.Remove(1)

    def draw_probes(self):
        """Draw all probes to the GUI."""
        self.clear_probes()
        probes = self.scope.get_probes()
        for probe in probes:
            self.add_probe(probe)
        self.SetupScrolling()
