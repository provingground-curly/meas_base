#
# LSST Data Management System
#
# Copyright 2008-2017  AURA/LSST.
#
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the LSST License Statement and
# the GNU General Public License along with this program.  If not,
# see <https://www.lsstcorp.org/LegalNotices/>.
#

from collections import Iterable

from lsst.afw.table import Schema, SourceCatalog
from lsst.meas.base import NoiseReplacer, NoiseReplacerConfig
from lsst.meas.base import SingleFrameMeasurementTask as SFMT  # noqa N814


def rebuildNoiseReplacer(exposure, measCat):
    """ Recreate NoiseReplacer used in measurement

    Given a measurement catalog and the exposure on which the measurements were
    made, reconstruct the NoiseReplacer object that was used to mask out
    sources during measurement.

    Parameters
    ----------
    exposure : lsst.awf.exposure.Exposure
        The exposure on which measurements were made

    meaCat : lsst.afw.table.SourceCatalog
        Catalog containing the outputs of measurements on each source

    Returns
    -------
    noiseReplacer : lsst.meas.base.NoiseReplacer
        Object used to replace and or restore sources in the exposure with
        deterministic noise
    """

    algMetadata = measCat.getMetadata()
    noiseReplacerConf = NoiseReplacerConfig()
    noiseReplacerConf.noiseSeedMultiplier = \
        algMetadata.getScalar(SFMT.NOISE_SEED_MULTIPLIER)
    noiseReplacerConf.noiseSource = algMetadata.getScalar(SFMT.NOISE_SOURCE)
    noiseReplacerConf.noiseOffset = algMetadata.getScalar(SFMT.NOISE_OFFSET)

    footprints = {src.getId(): (src.getParent(), src.getFootprint())
                  for src in measCat}

    try:
        exposureId = algMetadata.getScalar(SFMT.NOISE_EXPOSURE_ID)
    except Exception:
        exposureId = None

    noiseReplacer = NoiseReplacer(noiseReplacerConf, exposure, footprints,
                                  exposureId=exposureId)
    return noiseReplacer


def makeRerunCatalog(schema, oldCatalog, idList, fields=None,
                     resetParents=True, addParents=False):
    """ Creates a catalog prepopulated with ids

    This function is used to generate a SourceCatalog containing blank records
    with Ids specified in the idList parameter

    This function is primarily used when rerunning measurements on a footprint.
    Specifying ids in a new measurement catalog which correspond to ids in an
    old catalog makes comparing results much easier.

    The resetParents and addParents options are needed because
    lsst.meas.base.SingleFrameMeasurementTask.runPlugins() will skip child
    objects whose parents are not in the catalog.

    Parameters
    ----------
    schema : lsst.afw.table.Schema
        Schema used to describe the fields in the resulting SourceCatalog

    oldCatalog : lsst.afw.table.SourceCatalog
        Catalog containing previous measurements.

    idList : iterable
        Python iterable whose values should be numbers corresponding to
        measurement ids, ids must exist in the oldCatalog

    fields : iterable
        Python iterable whose entries should be strings corresponding to schema
        keys that exist in both the old catalog and input schema. Fields listed
        will be copied from the old catalog into the new catalog.

    resetParents: boolean
        Flag to toggle whether child objects whose parents are not in the
        idList should have their parents reset to zero.

    addParents: boolean
        Flag to toggle whether parents of child objects will be added to the
        idList (if not already present).

    Returns
    -------
    measCat : lsst.afw.table.SourceCatalog
        SourceCatalog prepopulated with entries corresponding to the ids
        specified
    """

    if not isinstance(schema, Schema):
        raise RuntimeError("schema must be an instance of "
                           "lsst.afw.table.Schema")

    if not isinstance(oldCatalog, SourceCatalog):
        raise RuntimeError("oldCatalog must be an instance of "
                           "lsst.afw.table.SourceCatalogiterable")

    if fields is None:
        fields = []
    if not isinstance(fields, Iterable):
        raise RuntimeError("fields list must be an iterable with string"
                           "elements")
    for entry in fields:
        if entry not in schema:
            schema.addField(oldCatalog.schema.find(entry).field)

    if addParents:
        newIdList = set()
        for srcId in idList:
            newIdList.add(srcId)
            parentId = oldCatalog.find(srcId).getParent()
            if parentId:
                newIdList.add(parentId)
        idList = newIdList
    idList = sorted(idList)

    measCat = SourceCatalog(schema)
    for srcId in idList:
        oldSrc = oldCatalog.find(srcId)
        src = measCat.addNew()
        src.setId(srcId)
        src.setFootprint(oldSrc.getFootprint())
        parent = oldSrc.getParent()
        if resetParents and parent and parent not in idList:
            parent = 0
        src.setParent(parent)
        src.setCoord(oldSrc.getCoord())
        for entry in fields:
            src[entry] = oldSrc[entry]
    return measCat
