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

from collections import OrderedDict
import json
import os
from pubsub import pub

import cachegrab
from .pipeline import Pipeline
from .sources import ScopeSource, DatasetSource
from .sinks import DatasetSink
from .analyses import AnalysisSimple, AnalysisGraph
from .data import Dataset

class Environment:
    """Contain all sources, sinks, datasets, and pipelines."""

    # Default dataset name
    DEFAULT_DATASET = "default"
    # Magic value to automatically preview most recent sample
    AUTO_PREVIEW = -2

    # Events for pubsub
    DATASETS_MODIFIED = "ENVIRONMENT_DATASETS_MODIFIED"
    DATASET_ACTIVATED = "ENVIRONMENT_DATASET_ACTIVATED"
    PREV_SAMP_CHANGED = "ENVIRONMENT_PREVIEW_SAMPLE_CHANGED"
    
    def __init__(self):
        """Initialize new environment."""
        self.scope_source = ScopeSource()
        self.analysis_sink = AnalysisGraph()
        self.datasets = {}
        self.names = []
        self.active_dataset = None
        
        self.preview_analysis = True
        self.set_preview_sample(None)
        
        # Add collection pipeline
        self.collection_pipeline = Pipeline(self.scope_source, None)

        # Add analysis pipeline
        self.analysis_pipeline = Pipeline(None, self.analysis_sink)

        # Add default dataset
        self.add_dataset(Dataset(), self.DEFAULT_DATASET)
        self.activate_dataset(self.DEFAULT_DATASET)

        self.savepath = None
        self.needsave = True
        
        pub.subscribe(self.sample_added, Dataset.SAMPLE_ADDED)

    def get_dataset_count(self):
        """Return the number of datasets."""
        return len(self.datasets)

    def get_dataset_names(self):
        """Return the names of the datasets."""
        return self.names

    def get_dataset_by_ind(self, ind):
        """Get the dataset at the numerical index"""
        name = self.names[ind]
        return name, self.datasets[name]

    def get_dataset_by_name(self, name):
        """Get the dataset with the specified name"""
        return self.datasets[name]

    def add_dataset(self, ds, name=None):
        """Add a dataset to the dictionary of datasets.

        Returns the name of the newly created dataset"""
        if name is None or len(name) == 0:
            name = self.DEFAULT_DATASET
        uniq_name = self._get_unique_dataset_name(name)
        self.datasets[uniq_name] = ds
        self.names.append(uniq_name)
        pub.sendMessage(Environment.DATASETS_MODIFIED)
        return uniq_name

    def rename_dataset(self, old_name, new_name=None):
        """Rename a dataset in the dictionary of datasets.

        The first argument is the old name of the dataset.
        The second is the new name of the dataset.
        Returns the name of the newly renamed dataset."""
        if new_name is None or len(new_name) == 0:
            new_name = self.DEFAULT_DATASET
        if old_name == new_name:
            return old_name
        old_ds = self.datasets[old_name]
        old_ind = self.names.index(old_name)
        del self.names[old_ind]
        del self.datasets[old_name]
        uniq_name = self._get_unique_dataset_name(new_name)
        self.datasets[uniq_name] = old_ds
        self.names.insert(old_ind, uniq_name)
        if self.active_dataset == old_name:
            self.activate_dataset(uniq_name)
            
        pub.sendMessage(Environment.DATASETS_MODIFIED)
        return uniq_name

    def remove_dataset(self, name):
        """Remove the dataset with the given name."""
        ind = self.names.index(name)
        del self.names[ind]
        del self.datasets[name]

        if self.active_dataset == name:
            self.activate_dataset(None)

        if len(self.names) == 0:
            # Add default dataset
            self.add_dataset(Dataset(), self.DEFAULT_DATASET)
            self.activate_dataset(self.DEFAULT_DATASET)
        pub.sendMessage(Environment.DATASETS_MODIFIED)

    def activate_dataset (self, name):
        """Activates the dataset of the given name.

        This sets up the collection and analysis pipeline to use this
        dataset. If name is None, then no dataset is activated."""
        if name is not None:
            ds = self.datasets[name]
            self.active_dataset = name
            self.collection_pipeline.set_sink(DatasetSink(ds))
            self.analysis_pipeline.set_source(DatasetSource(ds))
            self.set_preview_sample(Environment.AUTO_PREVIEW)
        else:
            self.active_dataset = None
            self.collection_pipeline.set_sink(None)
            self.analysis_pipeline.set_source(None)
            self.set_preview_sample(None)
        pub.sendMessage(Environment.DATASET_ACTIVATED)

    def get_active_dataset_name(self):
        """Returns the name of the active dataset."""
        return self.active_dataset

    def _get_unique_dataset_name(self, name):
        """Return a name that does not conflict with any current datasets."""
        if name not in self.names:
            return name

        ind = 1
        while True:
            new_name = "%s_%03d" % (name, ind)
            if new_name not in self.names:
                return new_name
            ind += 1

    def set_preview_analysis(self, val):
        """Sets the value of the preview_analysis flag."""
        self.preview_analysis = val
        self.set_preview_sample(None)

    def set_preview_sample(self, ind):
        """Sets which sample in the active dataset has the focus.

        If ind is None, then there is no active sample.
        If ind is AUTO_PREVIEW, then active sample is the most recent sample.
        If ind is something else, then that index sample is active"""
        self._preview_sample = ind

        self.preview_changed()

    def get_preview(self):
        """Returns the sample that currently has focus."""
        if self._preview_sample is None:
            return None

        name = self.get_active_dataset_name()
        ds = self.get_dataset_by_name(name)
        if self._preview_sample == self.AUTO_PREVIEW:
            return ds.get_sample(len(ds) - 1)
        else:
            return ds.get_sample(self._preview_sample)

    def sample_added(self):
        """Callback when a new sample has been added to the dataset."""
        if self._preview_sample == self.AUTO_PREVIEW:
            # If _preview_sample has this magic value and
            # we want to preview the sample
            self.preview_changed()

    def preview_changed(self):
        """Callback when the preview has changed.

        Indicates the analysis pipeline needs to be run for this sample."""
        if self.preview_analysis:
            samp = self.get_preview()
            if samp is not None:
                self.analysis_pipeline.run(samples=[samp])
            else:
                self.analysis_sink.clean()
        pub.sendMessage(Environment.PREV_SAMP_CHANGED)

    def save(self, path):
        """Save the Environment to the given location on the filesystem.

        The save file consists of a number of Datasets."""
        if os.path.exists(path) and not os.path.isdir(path):
            raise IOError("Save path is not a directory")
        if not os.path.exists(path):
            os.makedirs(path)

        ds_names = self.get_dataset_names()
        active_ds = self.active_dataset
        desc = {
            "version": cachegrab.__version__,
            "datasets": ds_names,
            "active_dataset": active_ds if active_ds is not None else "",
            "source": self.scope_source.save(),
            "analyzer": self.analysis_sink.save(),
            "collection_filters": [],
            "analysis_filters": [],
            "preview_analysis": self.preview_analysis,
            "preview_sample": self._preview_sample,
        }

        desc_loc = os.path.join(path, "environment.json")
        for filt in self.collection_pipeline.get_filters():
            filt_desc = filt.save()
            desc["collection_filters"].append(filt_desc)
        for filt in self.analysis_pipeline.get_filters():
            filt_desc = filt.save()
            desc["analysis_filters"].append(filt_desc)
        for ds_name in ds_names:
            ds_path = os.path.join(path, ds_name)
            self.get_dataset_by_name(ds_name).save(ds_path)
        file_contents = json.dumps(desc)
        with open(desc_loc, 'w') as f:
            f.write(file_contents)

        self.savepath = path

    def load(self, path):
        """Load the Environment from the given location on the filesystem.

        Returns a reference to itself."""
        if not os.path.exists(path) or os.path.isfile(path):
            raise IOError("Load path is not a directory")

        desc_loc = os.path.join(path, "environment.json")
        with open(desc_loc, 'r') as f:
            desc = json.loads(f.read())
            vers = desc["version"]
            if vers != cachegrab.__version__:
                raise IOError("Wrong file version (%s)!" % vers)
            self.load_v01(path, desc)
        self.savepath = path
        return self

    def load_v01(self, path, desc):
        """Load version 0.1 environment from the filesystem."""
        self.load_source(desc["source"])
        self.load_analyzer(desc["analyzer"])
        
        self.preview_analysis = desc["preview_analysis"]
        self.set_preview_sample(None)

        # Load pipelines
        self.load_pipelines(desc)

        # Load datasets
        self.datasets = {}
        self.names = []
        self.active_dataset = None
        ds_names = desc["datasets"]
        for ds_name in ds_names:
            ds_path = os.path.join(path, ds_name)
            ds = Dataset().load(ds_path)
            self.add_dataset(ds, ds_name)
        active_ds = desc["active_dataset"]
        if active_ds != "":
            self.activate_dataset(active_ds)

        self.set_preview_sample(desc["preview_sample"])

    def load_source(self, desc):
        """Load the sample source by the given description."""
        for cls in cachegrab.sources.get_sources():
            if cls.typename == desc["type"]:
                self.scope_source = cls().load(desc)
                return
        raise IOError("Unrecognized Source %s" % desc["type"])

    def load_analyzer(self, desc):
        """Load the analysis sink by the given description."""
        for cls in cachegrab.analyses.get_analyses():
            if cls.typename == desc["type"]:
                self.analysis_sink = cls().load(desc)
                return
        raise IOError("Unrecognized Analyzer %s" % desc["type"])

    def load_pipelines(self, desc):
        """Load in the filters for the collection and analysis pipelines."""
        self.collection_pipeline = Pipeline(self.scope_source, None)
        cfilts = desc["collection_filters"]
        for filt in cfilts:
            for cls in cachegrab.filters.get_filters():
                if cls.typename == filt["type"]:
                    f = cls().load(filt)
                    self.collection_pipeline.add_filter(f)

        self.analysis_pipeline = Pipeline(None, self.analysis_sink)
        afilts = desc["analysis_filters"]
        for filt in afilts:
            for cls in cachegrab.filters.get_filters():
                if cls.typename == filt["type"]:
                    try:
                        f = cls().load(filt)
                        self.analysis_pipeline.add_filter(f)
                    except KeyError as e:
                        pass
        
