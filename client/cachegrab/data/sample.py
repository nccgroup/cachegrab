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

import json
import os

import cachegrab

from trace import Trace

class Sample:
    """Contain the data collected from a single capture."""

    def __init__(self):
        """Constructor"""
        self.traces = {}
        self.extra = {}

    def copy(self):
        """Returns a duplicate of this object."""
        ret = Sample()
        for tname in self.traces.keys():
            ret.add_trace(tname, self.traces[tname].copy())
        for tag in self.extra.keys():
            ret.add_extra(tag, self.extra[tag])
        return ret

    def add_trace(self, name, trace):
        """Add the given trace to the sample."""
        self.set_trace(name, trace)

    def set_trace(self, name, trace):
        """Sets the given trace within the sample."""
        self.traces[name] = trace

    def get_trace(self, name):
        """Retrieve the trace from the sample."""
        return self.traces[name]

    def get_trace_names(self):
        """Return the names of the traces"""
        return self.traces.keys()

    def add_extra(self, tag, data):
        """Add extra data to the sample.

        It must be posible to JSON-encode data"""
        self.extra[tag] = data

    def get_extra(self, tag):
        """Retrieve the extra data"""
        return self.extra[tag]

    def get_extra_names(self):
        """Return the names of the extra data"""
        return self.extra.keys()

    def save(self, path):
        """Save the sample to the given path."""
        if os.path.exists(path) and not os.path.isdir(path):
            raise IOError("Save path is not a directory")
        if not os.path.exists(path):
            os.makedirs(path)

        desc = {
            "version": cachegrab.__version__,
            "traces": [],
            "extra": {},
        }
        desc_loc = os.path.join(path, "sample.json")
        with open(desc_loc, 'w') as f:
            for tname in self.traces.keys():
                fname = os.path.join(path, tname + ".trace")
                desc["traces"].append(tname)
                self.traces[tname].save(fname)
            for ename in self.extra.keys():
                desc["extra"][ename] = self.extra[ename]
            f.write(json.dumps(desc))

    def load(self, path):
        """Load a sample from the given path.

        Returns a reference to itself."""
        if not os.path.exists(path) or os.path.isfile(path):
            raise IOError("Load path is not a directory")

        desc_loc = os.path.join(path, "sample.json")
        with open(desc_loc) as f:
            desc = json.loads(f.read())
            vers = desc["version"]
            if vers != cachegrab.__version__:
                raise IOError("Wrong file version (%s)!" % vers)
            self.load_v01(path, desc)
        return self

    def load_v01(self, path, desc):
        """Load version 0.1 sample from the filesystem."""
        tnames = desc["traces"]
        for tname in tnames:
            fname = os.path.join(path, tname + ".trace")
            tr = Trace().load(fname)
            self.add_trace(tname, tr)

        for ename in desc["extra"]:
            data = desc["extra"][ename]
            self.add_extra(ename, data)
