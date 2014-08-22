// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2013 LSST Corporation.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#ifndef LSST_MEAS_BASE_NaiveCentroid_h_INCLUDED
#define LSST_MEAS_BASE_NaiveCentroid_h_INCLUDED

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "lsst/pex/config.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/meas/base/Inputs.h"
#include "lsst/meas/base/ResultMappers.h"

namespace lsst { namespace meas { namespace base {
/**
 *  @brief A C++ control class to handle NaiveCentroidAlgorithm's configuration
 */
class NaiveCentroidControl {
public:
    LSST_CONTROL_FIELD(background, double, "Value to subtract from the image pixel values");

    /**
     *  @brief Default constructor
     *
     *  All control classes should define a default constructor that sets all fields to their default values.
     */
    NaiveCentroidControl() : background(0.0) {}
};

/**
 *  @brief A class that calculates a centroid as a simple unweighted first moment
 *         of the 3x3 region around a pixel.
 *
 *   A fixed background (set via config) may optionally be subtracted.
 */

class NaiveCentroidAlgorithm {
public:

    /**
     *  @brief Flag bits to be used with the 'flags' data member of the Result object.
     *
     *  Inspect getFlagDefinitions() for more detailed explanations of each flag.
     */
    enum FlagBits {
        NO_COUNTS,
        EDGE,
        N_FLAGS
    };

    /**
     *  @brief Return an array of (name, doc) tuples that describes the flags and sets the names used
     *         in catalog schemas.
     */
    static boost::array<FlagDef,N_FLAGS> const & getFlagDefinitions() {
        static boost::array<FlagDef,N_FLAGS> const flagDefs = {{
                {"noCounts", "Object to be centroided has no counts"},
                {"edge", "Object too close to edge"}
            }};
        return flagDefs;
    }

    /// A typedef to the Control object for this algorithm, defined above.
    /// The control object contains the configuration parameters for this algorithm.
    typedef NaiveCentroidControl Control;

    /**
     *  The apply() method return type - a CentroidComponent.
     */
    typedef Result1<
        NaiveCentroidAlgorithm,
        CentroidComponent
    > Result;

    /// The result mapper for this algorithm is a simple CentroidComponentMapper
    typedef ResultMapper1<
        NaiveCentroidAlgorithm,
        CentroidComponentMapper
    > ResultMapper;


    /**
     *  In the actual overload of apply() used by the Plugin system, this is the only argument besides the
     *  Exposure being measured.  NaiveCentroidAlgorithm only needs a centroid, so we use FootprintCentroidInput.
     */
    typedef FootprintCentroidInput Input; // type passed to apply in addition to Exposure.

    /**
     *  @brief Create an object that transfers Result values to a record associated with the given schema
     */
    static ResultMapper makeResultMapper(
        afw::table::Schema & schema,
        std::string const & prefix,
        Control const & ctrl=Control()
    );

    /**
     *  @brief Measure the centroid of a source using the NaiveCentroid algorithm.
     */
    template <typename T>
    static void apply(
        afw::image::Exposure<T> const & exposure,
        afw::geom::Point2D const & position,
        Result & result,
        Control const & ctrl=Control()
    );

    /**
     *  @brief Apply the NaiveCentroid to a single source using the Plugin API.
     */
    template <typename T>
    static void apply(
        afw::image::Exposure<T> const & exposure,
        Input const & inputs,
        Result & result,
        Control const & ctrl=Control()
    );

};

}}} // namespace lsst::meas::base

#endif // !LSST_MEAS_BASE_NaiveCentroid_h_INCLUDED
