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

import numpy as np
import os

class Trace:
    """Contain the data from a single trace in a Sample."""

    def __init__(self, data=None):
        """Initialize Trace.

        data is a numpy array"""
        self.data = data

    def copy(self):
        """Returns a duplicate of this trace."""
        return Trace(np.copy(self.data))

    def save(self, fname):
        """Save the trace to the given file."""
        fname = fname + ".npy"
        if os.path.exists(fname) and not os.path.isfile(fname):
            raise IOError("Save path is not a file")
        np.save(fname, self.data)

    def load(self, fname):
        """Load the trace from the given file.

        Returns a reference to itself."""
        fname = fname + ".npy"
        if os.path.exists(fname) and not os.path.isfile(fname):
            raise IOError("Save path is not a file")
        self.data = np.load(fname)
        return self
