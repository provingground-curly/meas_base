// -*- lsst-c++ -*-
/*
 * LSST Data Management System
 * Copyright 2008-2014 LSST Corporation.
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

#ifndef LSST_MEAS_BASE_ApertureFlux_h_INCLUDED
#define LSST_MEAS_BASE_ApertureFlux_h_INCLUDED

#include "lsst/pex/config.h"
#include "lsst/afw/image/Exposure.h"
#include "lsst/afw/table/arrays.h"
#include "lsst/afw/table/Source.h"
#include "lsst/meas/base/Results.h"

namespace lsst { namespace meas { namespace base {

class ApertureFluxControl {
public:

    ApertureFluxControl();

    LSST_CONTROL_FIELD(
        radii, std::vector<double>,
        "Radius (in pixels) of apertures."
    );

    LSST_CONTROL_FIELD(
        maxSincRadius, double,
        "Maximum radius (in pixels) for which the sinc algorithm should be used instead of the "
        "faster naive algorithm.  For elliptical apertures, this is the minor axis radius."
    );

    LSST_CONTROL_FIELD(
        shiftKernel, std::string,
        "Warping kernel used to shift Sinc photometry coefficients to different center positions"
    );

};

class ApertureFluxAlgorithm {
public:

    /// @copydoc PsfFluxAlgorithm::FlagBits
    enum FlagBits {
        APERTURE_TRUNCATED=0,
        SINC_COEFFS_TRUNCATED,
        N_FLAGS
    };

    /// @copydoc PsfFluxAlgorithm::getFlagDefinitions()
    static boost::array<FlagDef,N_FLAGS> const & getFlagDefinitions() {
        static boost::array<FlagDef,N_FLAGS> const flagDefs = {{
                {"apertureTruncated", "aperture did not fit within the measurement image (fatal)"},
                {"sincCoeffsTruncated",
                 "the extended coeffs used by the sinc algorithm did not fit within the measurement image"},
            }};
        return flagDefs;
    }

    /// Typedef to the control object associated with this algorithm, defined above.
    typedef ApertureFluxControl Control;

    /// Result object returned by static methods.
    typedef Result1<ApertureFluxAlgorithm,FluxComponent> Result;

    //@{
    /**  Compute the flux (and optionally, uncertanties) within an aperture using Sinc photometry
     *
     *   The Sinc algorithm is slower than a naive aperture, but more accurate, in that it correctly
     *   handles sub-pixel aperture boundaries on well-sampled data.  This improved accuracy is most
     *   important for smaller apertures.
     *
     *   @param[in]   image                 Image or MaskedImage to be measured.  If a MaskedImage is
     *                                      provided, uncertainties will be returned as well as fluxes.
     *   @param[in]   ellipse               Ellipse that defines the outer boundary of the aperture.
     *   @param[in]   ctrl                  Control object.
     */
    template <typename T>
    static Result computeSincFlux(
        afw::image::Image<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    );
    template <typename T>
    static Result computeSincFlux(
        afw::image::MaskedImage<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    );
    //@}

    //@{
    /**  Compute the flux (and optionally, uncertanties) within an aperture using naive photometry
     *
     *   The naive algorithm just counts the flux in pixels whose centers lie within the aperture,
     *   ignoring the effects of sub-pixel aperture boundaries.
     *
     *   @param[in]   image                 Image or MaskedImage to be measured.  If a MaskedImage is
     *                                      provided, uncertainties will be returned as well as fluxes.
     *   @param[in]   ellipse               Ellipse that defines the outer boundary of the aperture.
     */
    template <typename T>
    static Result computeNaiveFlux(
        afw::image::Image<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    );
    template <typename T>
    static Result computeNaiveFlux(
        afw::image::MaskedImage<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    );
    //@}

    //@{
    /**  Compute the flux (and optionally, uncertanties) within an aperture using the algorithm
     *   determined by its size and the maxSincRadius control parameter.
     *
     *   This method delegates to computeSincFlux is the minor axis of the aperture is smaller than
     *   ctrl.maxSincRadius, and delegates to computeNaiveFlux otherwise.
     *
     *   @param[in]   image                 Image or MaskedImage to be measured.  If a MaskedImage is
     *                                      provided, uncertainties will be returned as well as fluxes.
     *   @param[in]   ellipse               Ellipse that defines the outer boundary of the aperture.
     *   @param[in]   ctrl                  Control object.
     */
    template <typename T>
    static Result computeFlux(
        afw::image::Image<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    ) {
        return (afw::geom::ellipses::Axes(ellipse.getCore()).getB() <= ctrl.maxSincRadius)
            ? computeSincFlux(image, ellipse, ctrl)
            : computeNaiveFlux(image, ellipse, ctrl);
    }
    template <typename T>
    static Result computeFlux(
        afw::image::MaskedImage<T> const & image,
        afw::geom::ellipses::Ellipse const & ellipse,
        Control const & ctrl=Control()
    ) {
        return (afw::geom::ellipses::Axes(ellipse.getCore()).getB() <= ctrl.maxSincRadius)
            ? computeSincFlux(image, ellipse, ctrl)
            : computeNaiveFlux(image, ellipse, ctrl);
    }
    //@}

    /**
     *  Construct the algorithm and add its fields to the given Schema.
     */
    explicit ApertureFluxAlgorithm(
        Control const & ctrl,
        std::string const & name,
        afw::table::Schema & schema
    );

    /**
     *  Measure the configured apertures on the given image.
     *
     *  @param[in,out] record      Record used to save outputs and retrieve positions.
     *  @param[in]     exposure    Image to be measured.
     */
    virtual void measure(
        afw::table::SourceRecord & record,
        afw::image::Exposure<float> const & exposure
    ) const = 0;

    virtual ~ApertureFluxAlgorithm() {}

protected:

    struct FlagKeys {

        FlagKeys(std::string const & name, afw::table::Schema & schema, int index);

        afw::table::Key<afw::table::Flag> failed;
        afw::table::Key<afw::table::Flag> apertureTruncated;
        afw::table::Key<afw::table::Flag> sincCoeffsTruncated;
    };

    void copyResultToRecord(Result const & result, afw::table::SourceRecord & record, int index) const;

    Control const _ctrl;
    afw::table::ArrayKey<Flux> _fluxKey;
    afw::table::ArrayKey<FluxErrElement> _fluxSigmaKey;
    std::vector<FlagKeys> _flagKeys;
};

}}} // namespace lsst::meas::base

#endif // !LSST_MEAS_BASE_ApertureFlux_h_INCLUDED
