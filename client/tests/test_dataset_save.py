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
import unittest

from .context import cachegrab

from cachegrab.data import Dataset, Sample, Trace

class DatasetSaveTest(unittest.TestCase):
    def test(self):
        trace = Trace(np.identity(5))

        samp = Sample()
        samp.add_trace("test", trace)
        samp.add_trace("test2", trace)

        samp2 = Sample()
        samp2.add_trace("test3", trace)

        ds = Dataset()
        ds.add_sample(samp)
        ds.add_sample(samp2)

        dir_ = "/tmp/foo"
        ds.save(dir_)
        
        ds2 = Dataset().load(dir_)
        for sample in ds2:
            for t in sample.get_trace_names():
                data = sample.get_trace(t).data
                self.assertTrue(np.all(data == np.identity(5)))
