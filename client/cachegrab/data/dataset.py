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
from pubsub import pub
import os

import cachegrab

from sample import Sample

class Dataset:
    """Represent a collection of samples.

    This interface is capable of exposing samples one at a time through
    a generator, which makes it easier to implement filters that can
    operate on single samples at a time, or must operate on the entire
    dataset."""

    # Different pubsub events
    SAMPLE_ADDED = "DATASET_SAMPLE_ADDED"

    def __init__(self):
        self._samples = []

    def __len__(self):
        """Return the number of samples in the dataset."""
        return len(self._samples)

    def __iter__(self):
        """ Return a generator of all samples."""
        return (s for s in self._samples)

    def add_sample(self, sample):
        """Add a sample to the dataset."""
        self._samples.append(sample)
        pub.sendMessage(Dataset.SAMPLE_ADDED)

    def add_samples(self, samples):
        """Add samples from a generator object."""
        for samp in samples:
            self.add_sample(samp)

    def get_sample(self, ind):
        """Return the sample at the specified offset."""
        if 0 <= ind < len(self._samples):
            return self._samples[ind]
        else:
            return None

    def save(self, path):
        """Export the dataset to the given location on the filesystem.

        The path argument specifies a directory. This function will create
        a dataset.json file and will contain numbered directories, one for
        each sample."""
        
        if os.path.exists(path) and not os.path.isdir(path):
            raise IOError("Save path is not a directory")
        if not os.path.exists(path):
            os.makedirs(path)

        nsamp = len(self._samples)
        desc = {
            "version": cachegrab.__version__,
            "number_of_samples": nsamp,
        }

        desc_loc = os.path.join(path, "dataset.json")
        with open(desc_loc, 'w') as f:
            f.write(json.dumps(desc))

            for i in range(nsamp):
                dname = "%06u" % i
                sample_path = os.path.join(path, dname)
                self._samples[i].save(sample_path)

    def load(self, path):
        """Load the dataset from the given location on the filestytem.

        Returns a reference to itself."""
        if not os.path.exists(path) or os.path.isfile(path):
            raise IOError("Load path is not a directory")

        desc_loc = os.path.join(path, "dataset.json")
        with open(desc_loc) as f:
            desc = json.loads(f.read())
            vers = desc["version"]
            if vers != cachegrab.__version__:
                raise IOError("Wrong file version (%s)!" % vers)
            self.load_v01(path, desc)
        return self

    def load_v01(self, path, desc):
        """Load version 0.1 dataset from the filesystem."""
        nsamp = desc["number_of_samples"]
        for i in range(nsamp):
            dname = "%06u" % i
            sample_path = os.path.join(path, dname)
            sample = Sample().load(sample_path)
            self.add_sample(sample)
