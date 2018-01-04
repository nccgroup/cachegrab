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

import wx

from .scope_config import ScopeConfig
from .dataset_explorer import DatasetExplorer
from .source_collector import SourceCollector
from .analysis_selector import AnalysisSelector

class LeftPanel(wx.Notebook):
    """Left side of the window.

    Includes the notebook of settings.
    Panels:
     - Scope Configuration
     - Dataset Explorer
    """

    def __init__(self, parent, env):
        """Initialize notebook"""
        wx.Notebook.__init__(self, parent, wx.ID_ANY)

        # Set up controller
        self.env = env

        # Set up view
        scope = ScopeConfig(self, self.env)
        self.AddPage(scope, "Scope")

        ds_explore = DatasetExplorer(self, self.env)
        self.AddPage(ds_explore, "Datasets")

        ana_select = AnalysisSelector(self, self.env)
        self.AddPage(ana_select, "Analysis")

        src_collect = SourceCollector(self, self.env)
        self.AddPage(src_collect, "Collection")
