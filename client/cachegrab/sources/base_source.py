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
import json

class CaptureSource(object):
    """Base class for the capture source."""

    typename = "Default_Source"

    # Pubsub events
    READINESS_CHANGED = "CAPTURE_SOURCE_READINESS_CHANGED"

    def __init__(self):
        """Initialize capture source."""
        self.set_ready(True)

    def set_ready(self, state):
        """Specifies that the capture source is ready to collect."""
        self._ready = state
        pub.sendMessage(CaptureSource.READINESS_CHANGED)

    def is_ready(self):
        """Returns true if the Source is ready to capture."""
        return self._ready
    
    def capture_once(self):
        """Return a single Sample from a newly executed capture."""
        raise NotImplementedError

    def capture_many(self, max_iter=None):
        """Capture samples and return a generator object.

        If max_iter is None, capture will continue until an error is
        raised. Otherwise this function yields at most max_iter samples."""
        if not self.is_ready():
            return
        
        if max_iter is None:
            while True:
                s = self.capture_once()
                if s is not None:
                    yield s
        else:
            for _ in range(max_iter):
                s = self.capture_once()
                if s is not None:
                    yield s

    def save(self):
        """Return a object representing the source's internal state.

        The object will be later encoded into JSON."""
        desc = {
            "type": self.typename,
        }
        return desc

    def load(self, desc):
        """Take the object and set the source's internal state.

        Returns a reference to the current object."""
        if desc["type"] != self.typename:
            raise TypeError("Unexpected Source type %s." % desc["type"])

        return self
