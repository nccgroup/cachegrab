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

from cachegrab.analyses import AnalysisSimpleGui, AnalysisGraphGui

class RightPanel(wx.Panel):
    """Right side of the window"""

    def __init__(self, parent, env):
        """Initialize"""
        wx.Panel.__init__(self, parent, wx.ID_ANY, style=wx.BORDER_SUNKEN)

        # Set up controller
        self.env = env

        # Add sizer for Analysis view
        sizer = wx.BoxSizer(wx.VERTICAL)

        analyzer_ctl = self.env.analysis_sink
        self.analyzer = AnalysisGraphGui(self, analyzer_ctl)
        sizer.Add(self.analyzer, 1, wx.EXPAND)

        sizer.SetSizeHints(self)
        self.SetSizer(sizer)
