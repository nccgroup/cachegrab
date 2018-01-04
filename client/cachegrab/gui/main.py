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

from ..environment import Environment

from left_panel import LeftPanel
from right_panel import RightPanel

TITLE="cachegrab"

class MainFrame(wx.Frame):
    """Main application frame.

    In the MVC model, this is a combination of the View and Controller"""
    
    def __init__(self, env):
        """Initialize"""
        # Initialize Controller
        self.env = env

        # Initialize View
        wx.Frame.__init__(self, None, title=TITLE, size=(640, 480))

        self.panel = wx.Panel(self)

        self._setup_menus()

        sizer = wx.BoxSizer(wx.HORIZONTAL)
        left = LeftPanel(self.panel, self.env)
        sizer.Add(left, 1, wx.EXPAND)

        right = RightPanel(self.panel, self.env)
        sizer.Add(right, 3, wx.EXPAND)

        sizer.SetSizeHints(self.panel)
        self.panel.SetSizer(sizer)

        self.retitle()
        self.Show()
        #self.Maximize()

    def _setup_menus(self):
        """Set up menu bar"""
        menu = wx.MenuBar()
        file_ = wx.Menu()
        new_ = file_.Append(wx.NewId(),
                            "&New\tCtrl+n",
                            "Create a new session")
        open_ = file_.Append(wx.NewId(),
                             "&Open\tCtrl+o",
                             "Open an existing session")
        save_ = file_.Append(wx.NewId(),
                             "&Save\tCtrl+s",
                             "Save the existing session")
        save_as_ = file_.Append(wx.NewId(),
                                "Save &As\tCtrl+Shift+s",
                                "Save the existing session")
        quit_ = file_.Append(wx.NewId(),
                             "&Quit\tCtrl+w",
                             "Quit the application")
        self.Bind(wx.EVT_MENU, self.onNew, new_)
        self.Bind(wx.EVT_MENU, self.onOpen, open_)
        self.Bind(wx.EVT_MENU, self.onSave, save_)
        self.Bind(wx.EVT_MENU, self.onSaveAs, save_as_)
        self.Bind(wx.EVT_MENU, self.onExit, quit_)
        self.Bind(wx.EVT_CLOSE, self.onClose)

        menu.Append(file_, "&File")
        self.SetMenuBar(menu)

    def retitle(self):
        if self.env.savepath is not None:
            title = "%s (%s)" % (TITLE, self.env.savepath)
            self.SetTitle(title)

    def onNew(self, evt):
        """Called when creating a new environment."""
        if self.promptSave():
            self.env.needsave = False
            self.Close()
            close_frame(self)
            env = Environment()
            open_frame(env)

    def onOpen(self, evt):
        """Called when opening an existing environment."""
        if not self.promptSave():
            return
        
        dlg = wx.DirDialog(self, "Choose a directory to open:")
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            try:
                env = Environment().load(path)
            except IOError as e:
                err = wx.MessageDialog(
                    self,
                    'Failed to open %s.' % path,
                    'Error',
                    wx.OK
                )
                err.ShowModal()
                err.Destroy()
            else:
                # Don't need to save, so don't show dialog
                self.env.needsave = False
                self.Close()
                close_frame(self)
                open_frame(env)
        dlg.Destroy()
        self.retitle()

    def onSave(self, evt):
        if self.env.savepath is None:
            path = self.saveDialog()
        else:
            path = self.env.savepath
        self.doSave(path)
        self.retitle()

    def onSaveAs(self, evt):
        path = self.saveDialog()
        self.doSave(path)
        self.retitle()

    def onExit(self, evt):
        """Called when the main frame is exiting."""
        self.Close()

    def onClose(self, evt):
        """Called when the main frame is closing."""
        if self.promptSave():
            self.Destroy()

    def saveDialog(self):
        """Show the actual dialog for saving the session.

        Returns the selected path if successful."""
        path = None
        dlg = wx.DirDialog(self, "Choose a directory to save to:")
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
        dlg.Destroy()
        return path

    def doSave(self, path):
        """Try to save the environment to the path.

        This will raise a dialog on failure.
        Returns True on success."""
        if path is None:
            return False
        try:
            self.env.save(path)
        except IOError as e:
            err = wx.MessageDialog(
                self,
                'Failed to save in %s.' % path,
                'Error',
                wx.OK
            )
            err.ShowModal()
            err.Destroy()
            return False
        else:
            msg = wx.MessageDialog(
                self,
                'Successfully saved to %s.' % path,
                'Success',
                wx.OK
            )
            msg.ShowModal()
            msg.Destroy()
            return True


    def promptSave(self):
        """Prompt the user to save the current session.

        Returns True if the user wishes to continue."""
        if not self.env.needsave:
            return True
        
        rv = False
        dlg = wx.MessageDialog(
            self,
            'Would you like to save the current project?',
            'Save Project',
            wx.YES | wx.NO | wx.CANCEL
        )
        resp = dlg.ShowModal()
        if resp == wx.ID_YES:
            if self.env.savepath is None:
                path = self.saveDialog()
            else:
                path = self.env.savepath
            rv = self.doSave(path)
        elif resp == wx.ID_NO:
            rv = True
        dlg.Destroy()
        return rv

open_frames = []
def open_frame(env):
    """Opens a new frame based on the environment."""
    global open_frames
    main = MainFrame(env)
    open_frames.append(main)

def close_frame(fr):
    """Closes an existing frame."""
    global open_frames
    open_frames.remove(fr)

def launch (fname=None):
    app = wx.App()

    env = Environment()
    if fname is not None:
        env.load(fname)
    open_frame(env)

    app.MainLoop()
