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

import os
from setuptools import setup, find_packages

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name = "cachegrab",
    version = "0.1.0",
    author = "Keegan Ryan",
    author_email = "keegan.ryan@nccgroup.trust",
    description = ("A utility to collect and analyze data from "
                   "microarchitectural attacks."),
    license = "GPLv2",
    keywords = "cache microarchitectural attack sidechannel",
    packages=find_packages(),
    long_description=read('README'),
    test_suite="tests",
    scripts=["scripts/cachegrab_gui"],
)
