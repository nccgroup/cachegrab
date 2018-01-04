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

import unittest

from .context import cachegrab

from cachegrab import Pipeline
from cachegrab.data import Dataset

from common import TestSource, TestSink, TestFilter

class PipelineTest(unittest.TestCase):
    def test(self):
        test_len = 4
        src = TestSource()
        filt = TestFilter()
        sink = TestSink()
        ds = Dataset()
        
        pipe1 = Pipeline(src, cachegrab.sinks.DatasetSink(ds))
        pipe1.add_filter(filt)
        pipe1.run(test_len)

        pipe2 = Pipeline(cachegrab.sources.DatasetSource(ds), sink)
        pipe2.run()
        
        self.assertEqual(sink.vals, [0, -1, -2, -3])
