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

class Pipeline:
    """A pipeline involving capture, filter, and analysis stages."""

    # Pubsub events
    FILTERS_CHANGED = "PIPELINE_FILTERS_CHANGED"
    
    def __init__(self, source, sink):
        """Initialize the capture pipeline.

        Keyword arguments:
        source -- the capture source object.
        sink -- the analysis sink object.
        """
        self.set_source(source)
        self.set_sink(sink)
        self._filters = []

    def set_source(self, source):
        """Set the capture source object."""
        self._source = source

    def set_sink(self, sink):
        """Set the analysis sink object."""
        self._sink = sink

    def add_filter(self, filter_):
        """Add a filter to the list of filters."""
        self._filters.append(filter_)
        pub.sendMessage(Pipeline.FILTERS_CHANGED, pipeline=self)

    def remove_filter(self, filter_):
        """Remove the specified filter"""
        self._filters.remove(filter_)
        pub.sendMessage(Pipeline.FILTERS_CHANGED, pipeline=self)

    def move_filter(self, filter_, dx):
        """Move the specified filter by the specified offset"""
        orig_ind = self._filters.index(filter_)
        new_ind = orig_ind + dx
        new_ind = max(0, min(len(self._filters), new_ind))
        filt = self._filters.pop(orig_ind)
        self._filters.insert(new_ind, filt)
        pub.sendMessage(Pipeline.FILTERS_CHANGED, pipeline=self)

    def get_filters(self):
        """Get the list of filters."""
        return self._filters

    def run(self, max_iter=None, samples=None):
        """Run the pipeline for the maximum number of iterations.

        Keyword arguments:
        max_iter -- The maximum number of samples to run through
                    the pipeline. By default the pipeline will until
                    the source stops producing.
        samples  -- If not None, use these samples instead of collecting
                    from the source. Otherwise it is an iterable of
                    Samples.
        """
        if samples is None:
            samples = self._source.capture_many(max_iter)
        for f in self._filters:
            samples = f.filter_many(samples)
        self._sink.analyze_many(samples)
