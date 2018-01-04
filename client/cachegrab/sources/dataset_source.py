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

from .base_source import CaptureSource

class DatasetSource(CaptureSource):
    """Retrieve the samples from a dataset."""

    typename = "Dataset_Source"

    def __init__(self, ds):
        """Initialize the source object."""
        CaptureSource.__init__(self)
        self._dataset = ds

    def capture_many(self, max_iter=None):
        """Retrieve up to max_iter samples from the dataset"""
        count = 0

        # Convert self._dataset to a list to prevent shenanigans where
        # a dataset is both the source and a sink in the pipeline.
        for sample in list(self._dataset):
            if max_iter is not None and count >= max_iter:
                break
            yield sample
            count += 1
