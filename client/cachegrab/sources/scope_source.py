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
import binascii
import itertools
import json
from pubsub import pub
import urllib

import numpy as np
import png

from ..data import Sample, Trace
from .base_source import CaptureSource

class Probe:
    """Represent a single probe"""

    # Pubsub events
    ENABLED_CHANGED = "PROBE_ENABLED_CHANGED"
    CONFIG_CHANGED = "PROBE_CONFIG_CHANGED"

    def __init__(self, scope, name):
        """Initialize probe"""
        self.scope = scope
        self.name = name

        self.associativity = 1
        self.num_sets = 1
        self.line_size = 1
        self.low_set = -1
        self.high_set = -1
        
        self._enabled = False

    def synchronize(self):
        """Synchronize the probe with the server."""
        url = "/%s/configuration" % self.name
        resp = self.scope.request(url)
        if resp_ok(resp):
            self.synchronize_desc(resp["probe"])

    def enable(self, assoc, nset, lsize):
        """Enable the probe"""
        self.associativity = assoc
        self.num_sets = nset
        self.line_size = lsize

        url = "/%s/connect" % self.name
        self.scope.request(url,
                           num_sets = self.num_sets,
                           associativity = self.associativity,
                           line_size = self.line_size)
        if self.low_set >= 0 and self.high_set >= 0:
            self.configure(self.low_set, self.high_set)
        self.synchronize()
        return self._enabled

    def disable(self):
        """Disable the probe"""
        url = "/%s/disconnect" % self.name
        self.scope.request(url, post = True)
        self.synchronize()

    def synchronize_desc(self, desc):
        """Synchronize the probe with the server by description."""
        if len(desc) == 0:
            self._enabled = False
            pub.sendMessage(self.ENABLED_CHANGED, name=self.name)
        else:
            self._enabled = True
            shape = desc["cache_shape"]
            self.associativity = shape["associativity"]
            self.num_sets = shape["num_sets"]
            self.line_size = shape["line_size"]
            if self.low_set < 0:
                self.low_set = self.num_sets - 1
            if self.high_set < 0:
                self.high_set = self.num_sets - 1
            pub.sendMessage(self.ENABLED_CHANGED, name=self.name)

            config = desc["config"]
            self.low_set = config["set_start"]
            self.high_set = config["set_end"]
            pub.sendMessage(self.CONFIG_CHANGED)

    def configure(self, low, high):
        """Configure the probe.

        Captures cache sets [low, high). If low > high, then the
        collection wraps around.
        Return if the configuration succeeded."""
        if not (0 <= low < self.num_sets):
            return False
        if not (0 < high <= self.num_sets):
            return False
        if high == low:
            return False

        url = "/%s/configuration" % self.name
        resp = self.scope.request(url,
                                  set_start = low,
                                  set_end = high)
        self.synchronize()
        return resp_ok(resp)

    def is_enabled(self):
        """Return whether the probe is enabled or not."""
        return self._enabled

    def save(self):
        """Return a object representing the probe's internal state.

        The object will be later encoded into JSON."""
        desc = {
            "associativity": self.associativity,
            "num_sets": self.num_sets,
            "line_size": self.line_size,
            "config_set_low": self.low_set,
            "config_set_high": self.high_set,
        }
        return desc

    def load(self, desc):
        """Take the object and set the probe's internal state.

        Returns a reference to the current object."""
        self.associativity = desc["associativity"]
        self.num_sets = desc["num_sets"]
        self.line_size = desc["line_size"]
        self.low_set = desc["config_set_low"]
        self.high_set = desc["config_set_high"]
        return self

class ScopeSource(CaptureSource):
    """Retrieve the samples from a dataset."""

    typename = "Scope_Source"
    
    # Pubsub events
    ENABLED_CHANGED = "SCOPE_SOURCE_ENABLED_CHANGED"
    CAPTURE_SETTINGS_CHANGED = "SCOPE_CAPTURE_SETTINGS_CHANGED"
    CONNECTED_CHANGED = "SCOPE_SOURCE_CONNECTED_CHANGED"
    PROBES_CHANGED = "SCOPE_SOURCE_PROBES_CHANGED"

    def __init__(self):
        """Initialize the source object."""
        CaptureSource.__init__(self)

        self.probes = {}

        self.set_ready(False)

        self._enabled = False
        self._connected = False
        self.server = "localhost:8000"

        # We want configurations to be in the save file, but instead
        # of applying the configurations at file load, load them as that
        # probe comes online
        self._saved_config = {}
        self._probe_configs = {}

        self._tcpu = 0
        self._scpu = 0
        self.num_cores = 0

        self.stalling_cutoff = 10000000
        self.timeout = 10000
        self.max_samples = 1000
        self.time_delta = 3000
        self.ta_command = ""
        self.ta_name = ""
        self.ta_cbuf = ""
        self.debug = False

    def _retrieve_measurement(self, type_):
        """Get the measurements for the specified type of probe."""
        location = "/capture/%s.png" % type_
        try:
            f = urllib.urlopen(self._host + location)
            resp = f.read()

            r = png.Reader(bytes=resp)
            width, height, pixels, meta = r.read()
            arr = np.vstack(itertools.imap(np.uint8, pixels))
            return arr
        except IOError as e:
            self.disconnect()
        return np.empty([0,0])

    def set_capture_params(self,
                           max_samples=None, time_delta=None,
                           ta_command=None, ta_name=None,
                           ta_cbuf=None, debug=None
    ):
        """Set the parameters for capture."""
        self.stalling_cutoff = 10000000
        self.timeout = 100000
        
        if max_samples is not None:
            self.max_samples = max_samples
        if time_delta is not None:
            self.time_delta = time_delta
        if ta_command is not None:
            self.ta_command = ta_command
        if ta_name is not None:
            self.ta_name = ta_name
        if ta_cbuf is not None:
            self.ta_cbuf = ta_cbuf
        if debug is not None:
            self.debug = debug

    def capture_once(self):
        """Return a sample from the scope"""
        resp = self.request("/capture/start",
                            max_samples=self.max_samples,
                            stalling_cutoff=self.stalling_cutoff,
                            time_delta=self.time_delta,
                            timeout=self.timeout,
                            command=self.ta_command,
                            ta_name=self.ta_name,
                            trigger_cbuf=self.ta_cbuf,
                            debug="y" if self.debug else "n"
        )
        s = Sample()
        if resp is None or not resp_ok(resp):
            return None
        
        nsamp = resp["num_samples"]
        
        s.add_extra("time_delta", self.time_delta)
        s.add_extra("command", self.ta_command)
        s.add_extra("ta_name", self.ta_name)
        s.add_extra("trigger_cbuf", self.ta_cbuf)
        s.add_extra("return_code", resp["return_code"])

        stdout = base64.b64encode(binascii.unhexlify(resp["stdout"]))
        s.add_extra("stdout", stdout)

        stderr = base64.b64encode(binascii.unhexlify(resp["stderr"]))
        s.add_extra("stderr", stderr)

        types = [t for t in self.probes.keys() if self.probes[t].is_enabled()]
        for type_ in types:
            if nsamp > 0:
                tr = Trace(self._retrieve_measurement(type_))
            else:
                tr = Trace(np.empty([0,0]))
            s.add_trace(type_, tr)
        return s

    def add_probe(self, name):
        """Add a probe to the scope.

        If we have a saved configuration for that probe, apply it."""
        p = Probe(self, name)
        self.probes[name] = p
        if name in self._probe_configs:
            p.load(self._probe_configs[name])
        pub.sendMessage(self.PROBES_CHANGED)

    def clear_probes(self):
        """Remove all probes."""
        for name in self.probes.keys():
            p = self.probes[name]
            self._probe_configs[name] = p.save()
        self.probes = {}
        pub.sendMessage(self.PROBES_CHANGED)
        
    def get_probes(self):
        """Return a list of probes allowed by the scope."""
        return self.probes.values()

    def get_target_core(self):
        """Return the target core"""
        if self._tcpu < self.num_cores:
            return self._tcpu
        else:
            return 0

    def set_target_core(self, cpu):
        """Set the target core"""
        self._tcpu = cpu

    def get_scope_core(self):
        """Return the scope core"""
        if self._scpu < self.num_cores:
            return self._scpu
        else:
            return 0

    def set_scope_core(self, cpu):
        """Set the scope core"""
        self._scpu = cpu

    def enable(self, target_core=None, scope_core=None):
        """Enable the scope"""
        if self._enabled:
            return True
        
        if target_core is not None:
            self.set_target_core(target_core)
        if scope_core is not None:
            self.set_scope_core(scope_core)
        tc = self.get_target_core()
        sc = self.get_scope_core()

        resp = self.request("/connect", target_cpu=tc, scope_cpu=sc)
        if resp_ok(resp):
            self._enabled = True
            self.synchronize()
            self.set_ready(self._enabled)
        return self._enabled

    def disable(self):
        """Disable the scope"""
        resp = self.request("/disconnect", post=True)
        self.synchronize()
        self.set_ready(self._enabled)

    def is_enabled(self):
        """Return whether the scope is enabled or not."""
        return self._enabled

    def connect(self, server):
        """Connect the scope to a particular server"""
        self._host = "http://%s" % server
        resp = self.request("/system")

        if resp_ok(resp):
            self.num_cores = resp["num_cores"]
            self.server = server
            self._connected = True
            self.synchronize()
            pub.sendMessage(self.CONNECTED_CHANGED)
        return self._connected

    def disconnect(self):
        """The connection to the server has been lost"""
        if not self._connected:
            return
        self.clear_probes()
        self._enabled = False
        self._connected = False
        self.num_cores = 0
        self.set_ready(False)
        
        pub.sendMessage(self.CONNECTED_CHANGED)

    def synchronize(self):
        """Synchronize the client's scope with the server's"""
        self.clear_probes()

        resp = self.request("/configuration")
        if not resp_ok(resp):
            pass
        
        desc = resp["scope"]
        if len(desc) == 0:
            self._enabled = False
            pub.sendMessage(self.ENABLED_CHANGED)
        else:
            self._enabled = True
            self.set_target_core(desc["target_cpu"])
            self.set_scope_core(desc["scope_cpu"])
            pub.sendMessage(self.ENABLED_CHANGED)
        
            for probename in desc["probes"].keys():
                if probename not in self.probes:
                    self.add_probe(probename)
                    self.probes[probename].synchronize_desc(
                        desc["probes"][probename])
        self.set_ready(self._enabled)

    def is_connected(self):
        """Return whether the scope is connected or not."""
        return self._connected

    def request(self, path, post=False, **kwargs):
        """Send a request to the scope."""
        if self._host is None:
            return None
        try:
            if len(kwargs) > 0 or post:
                params = urllib.urlencode(kwargs)
                f = urllib.urlopen(self._host + path, params)
            else:
                f = urllib.urlopen(self._host + path)
            raw = f.read()
            resp = json.loads(raw)
            return resp
        except IOError as e:
            self.disconnect()
            return None

    def save(self):
        """Return a object representing the source's internal state.

        The object will be later encoded into JSON."""
        desc = CaptureSource.save(self)

        desc["server"] = self.server
        desc["target_core"] = self._tcpu
        desc["scope_core"] = self._scpu

        probes = {}
        for key in self._probe_configs.keys():
            probes[key] = self._probe_configs[key]
        for key in self.probes.keys():
            p = self.probes[key]
            probes[key] = p.save()
        desc["probes"] = probes

        cap_params = {}
        cap_params["stalling_cutoff"] = self.stalling_cutoff
        cap_params["timeout"] = self.timeout
        cap_params["max_samples"] = self.max_samples
        cap_params["time_delta"] = self.time_delta
        cap_params["ta_command"] = self.ta_command
        cap_params["ta_name"] = self.ta_name
        cap_params["ta_cbuf"] = self.ta_cbuf
        cap_params["debug_tee_calls"] = "y" if self.debug else "n"
        desc["capture_params"] = cap_params

        return desc

    def load(self, desc):
        """Take the object and set the source's internal state.

        Returns a reference to the current object."""
        CaptureSource.load(self, desc)

        self.server = desc["server"]
        self._tcpu = desc["target_core"]
        self._scpu = desc["scope_core"]
        self._probe_configs = desc["probes"]

        cap_params = desc["capture_params"]
        self.stalling_cutoff = cap_params["stalling_cutoff"]
        self.timeout = cap_params["timeout"]
        self.max_samples = cap_params["max_samples"]
        self.time_delta = cap_params["time_delta"]
        self.ta_command = cap_params["ta_command"]
        self.ta_name = cap_params["ta_name"]
        self.ta_cbuf = cap_params["ta_cbuf"]
        self.debug = cap_params["debug_tee_calls"] == "y"

        return self

def resp_ok(resp):
    """Returns if the response from the server was "status: Success" """
    return resp is not None and resp["status"] == "Success"
