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

class AnalysisSink(object):
    """Base class for the analysis sink."""

    typename = "Default Sink"

    def analyze_once(self, sample):
        """Perform analysis on a single sample."""
        raise NotImplementedError

    def analyze_many(self, samples):
        """Analyze the samples from the pipeline."""
        for s in samples:
            self.analyze_once(s)

    def save(self):
        """Return a object representing the sink's internal state.

        The object will be later encoded into JSON."""
        desc = {
            "type": self.typename,
        }
        return desc

    def load(self, desc):
        """Take the object and set the sink's internal state.

        Returns a reference to the current object."""
        if desc["type"] != self.typename:
            raise TypeError("Unexpected Sink type %s." % desc["type"])

        return self
