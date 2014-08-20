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

#ifndef LSST_MEAS_BASE_GaussianFlux_h_INCLUDED
#define LSST_MEAS_BASE_GaussianFlux_h_INCLUDED

#include "lsst/pex/config.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/meas/base/Inputs.h"
#include "lsst/meas/base/ResultMappers.h"
#include "lsst/meas/base/algorithms/SdssShapeImpl.h"

namespace lsst { namespace meas { namespace base {

/**
 *  @brief A C++ control class to handle GaussianFluxAlgorithm's configuration
 */
class GaussianFluxControl {
public:
    LSST_CONTROL_FIELD(fixed, bool,
                       "if true, use existing shape and centroid measurements instead of fitting");
    LSST_CONTROL_FIELD(background, double, "FIXME! NEVER DOCUMENTED!");
    LSST_CONTROL_FIELD(shiftmax, double, "FIXME! NEVER DOCUMENTED!");
    LSST_CONTROL_FIELD(centroid, std::string, "name of centroid field to use if fixed is true");
    LSST_CONTROL_FIELD(shape, std::string, "name of shape field to use if fixed is true");
    LSST_CONTROL_FIELD(shapeFlag, std::string, "suffix of shape field flag to check if fixed is true");
    LSST_CONTROL_FIELD(maxIter, int, "Maximum number of iterations");
    LSST_CONTROL_FIELD(tol1, float, "Convergence tolerance for e1,e2");
    LSST_CONTROL_FIELD(tol2, float, "Convergence tolerance for FWHM");
    LSST_CONTROL_FIELD(usePixelWeights, bool, "Whether to use per-pixel inverse variance as weights");
    LSST_CONTROL_FIELD(badMaskPlanes, std::vector<std::string>,
                       "Mask planes that indicate pixels that should be excluded from the fit");

    /**
     *  @brief Default constructor
     *
     *  All control classes should define a default constructor that sets all fields to their default values.
     */
    GaussianFluxControl() :
        fixed(false), background(0.0), shiftmax(10.0),
        centroid("shape.sdss.centroid"), shape("shape.sdss"), shapeFlag(".flags"),
        maxIter(algorithms::SDSS_SHAPE_MAX_ITER),
        tol1(algorithms::SDSS_SHAPE_TOL1), tol2(algorithms::SDSS_SHAPE_TOL2)
    {}
};


/**
 *  @brief A measurement algorithm that estimates flux using an elliptical Gaussian weight.
 *
 *  This algorithm computes flux as the dot product of an elliptical Gaussian weight function
 *  with the image.  The size and ellipticity of the weight function is determined using the
 *  SdssShape algorithm, or retreived from a named field.
 */
class GaussianFluxAlgorithm {
public:

    /**
     *  @brief Flag bits to be used with the 'flags' data member of the Result object.
     *
     *  Inspect getFlagDefinitions() for more detailed explanations of each flag.
     */
    enum FlagBits {
        NO_PSF=0,
        NO_GOOD_PIXELS,
        EDGE,
        NO_FIXED,
        N_FLAGS
    };

    /**
     *  @brief Return an array of (name, doc) tuples that describes the flags and sets the names used
     *         in catalog schemas.
     */
    static boost::array<FlagDef,N_FLAGS> const & getFlagDefinitions() {
        static boost::array<FlagDef,N_FLAGS> const flagDefs = {{
                {"noPsf", "No Psf object attached to the Exposure object being measured"},
                {"noGoodPixels", "No usable pixels in fit region"},
                {"edge", "Could not use full PSF model image in fit because of proximity to exposure border"},
                {"noFixed", "Fixed option has been deprecated"}
            }};
        return flagDefs;
    }

    /// A typedef to the Control object for this algorithm, defined above.
    /// The control object contains the configuration parameters for this algorithm.
    typedef GaussianFluxControl Control;

    /**
     *  Result is the type returned by apply().  Because GaussianFluxAlgorithm only measures a flux and its
     *  uncertainty, we can use the single predefined component, FluxComponent, without modification.
     */
    typedef Result1<GaussianFluxAlgorithm,FluxComponent> Result;

    /**
     *  Use the FluxComponentMapper to map algorithm Result to output catalog
     */
    typedef ResultMapper1<GaussianFluxAlgorithm,FluxComponentMapper> ResultMapper;

    /**
     *  GaussianFluxAlgorithm only needs a centroid and footprint as input.
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
     *  @brief Measure the flux of a source using the GaussianFlux algorithm.
     */
    template <typename T>
    static void apply(
        afw::image::Exposure<T> const & exposure,
        afw::geom::Point2D const & position,
        Result & result,
        Control const & ctrl=Control()
    );

    /**
     *  @brief Apply the GaussianFlux to a single source using the Plugin API.
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

#endif // !LSST_MEAS_BASE_GaussianFlux_h_INCLUDED
