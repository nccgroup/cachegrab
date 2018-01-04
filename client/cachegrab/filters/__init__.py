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

from .base_filter import Filter, FilterGui
from .normalize import NormalizeFilter, NormalizeFilterGui
from .threshold import ThresholdFilter, ThresholdFilterGui
from .inclusion import InclusionFilter, InclusionFilterGui

def get_filters():
    """Return a list of possible filters"""
    return [
        NormalizeFilter,
        ThresholdFilter,
        InclusionFilter,
    ]

def get_filter_gui(filter_):
    """Return the *Gui class corresponding to a particular Filter object"""
    return {
        NormalizeFilter: NormalizeFilterGui,
        ThresholdFilter: ThresholdFilterGui,
        InclusionFilter: InclusionFilterGui,
    }[filter_.__class__]
