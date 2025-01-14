// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#ifndef   PANZER_EVALUATOR_GRADBASISCROSSVECTOR_DECL_HPP
#define   PANZER_EVALUATOR_GRADBASISCROSSVECTOR_DECL_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  Include Files
//
///////////////////////////////////////////////////////////////////////////////

// C++
#include <string>

// Kokkos
#include "Kokkos_DynRankView.hpp"

// Panzer
#include "Panzer_EvaluatorStyle.hpp"
#include "Panzer_Evaluator_WithBaseImpl.hpp"

// Phalanx
#include "Phalanx_Evaluator_Derived.hpp"
#include "Phalanx_MDField.hpp"

namespace panzer
{
  /**
   *  \brief Computes \f$ Ma(x)b(x)\cdots\int(\nabla\phi(x))\times
   *         \vec{s}(x)\,dx \f$.
   *
   *  Evaluates the integral
   *  \f[
   *    Ma(x)b(x)\cdots\int(\nabla\phi(x))\times\vec{s}(x)\,dx,
   *  \f]
   *  where \f$ M \f$ is some constant, \f$ a(x) \f$, \f$ b(x) \f$, etc., are
   *  some fields that depend on position, \f$ \vec{s} \f$ is some
   *  vector-valued function, and \f$ \phi \f$ is some vector basis.
   */
  template<typename EvalT, typename Traits>
  class Integrator_GradBasisCrossVector
    :
    public panzer::EvaluatorWithBaseImpl<Traits>,
    public PHX::EvaluatorDerived<EvalT, Traits>
  {
    public:

      /**
       *  \brief Main Constructor.
       *
       *  Creates an `Evaluator` to evaluate the integral
       *  \f[
       *    Ma(x)b(x)\cdots\int(\nabla\phi(x))\times\vec{s}(x)\,dx,
       *  \f]
       *  where \f$ M \f$ is some constant, \f$ a(x) \f$, \f$ b(x) \f$, etc.,
       *  are some fields that depend on position, \f$ \vec{s} \f$ is some
       *  vector-valued function, and \f$ \phi \f$ is some vector basis.
       *
       *  \param[in] evalStyle  An `enum` declaring the behavior of this
       *                        `Evaluator`, which is to either:
       *                        - compute and contribute (`CONTRIBUTES`), or
       *                        - compute and store (`EVALUATES`).
       *  \param[in] resNames   The names of either the contributed or
       *                        evaluated fields, depending on `evalStyle`.
       *                        This `Evaluator` computes either two or three
       *                        fields, depending on the dimensionality of \f$
       *                        \nabla\phi \f$.
       *  \param[in] vecName    The name of the vector-valued function being
       *                        integrated (\f$ \vec{s} \f$).
       *  \param[in] basis      The basis that you'd like to use (\f$ \phi
       *                        \f$).
       *  \param[in] ir         The integration rule that you'd like to use.
       *  \param[in] multiplier The scalar multiplier out in front of the
       *                        integral you're computing (\f$ M \f$).  If not
       *                        specified, this defaults to 1.
       *  \param[in] fmNames    A list of names of fields that are multipliers
       *                        out in front of the integral you're computing
       *                        (\f$ a(x) \f$, \f$ b(x) \f$, etc.).  If not
       *                        specified, this defaults to an empty `vector`.
       *  \param[in] vecDL      The vector data layout that you'd like to use.
       *                        If not specified, this defaults to
       *                        `Teuchos::null` and the vector data layout from
       *                        the given `ir` is used.
       *
       *  \throws std::invalid_argument If any of the inputs are invalid.
       *  \throws std::logic_error      If the `basis` supplied does not
       *                                support the gradient operator, or if
       *                                the dimension of the space exceeds the
       *                                dimension of the vector data layout, or
       *                                if has fewer components than the
       *                                dimensionality of the mesh.
       */
      Integrator_GradBasisCrossVector(
        const panzer::EvaluatorStyle&        evalStyle,
        const std::vector<std::string>&      resNames,
        const std::string&                   vecName,
        const panzer::BasisIRLayout&         basis,
        const panzer::IntegrationRule&       ir,
        const double&                        multiplier = 1,
        const std::vector<std::string>&      fmNames    =
          std::vector<std::string>(),
        const Teuchos::RCP<PHX::DataLayout>& vecDL      = Teuchos::null);

      /**
       *  \brief `ParameterList` Constructor.
       *
       *  Creates an `Evaluator` to evaluate the integral
       *  \f[
       *    Ma(x)b(x)\cdots\int(\nabla\phi(x))\times\vec{s}(x)\,dx,
       *  \f]
       *  where \f$ M \f$ is some constant, \f$ a(x) \f$, \f$ b(x) \f$, etc.,
       *  are some fields that depend on position, \f$ \vec{s} \f$ is some
       *  vector-valued function, and \f$ \phi \f$ is some basis.
       *
       *  \note This constructor exists to preserve the older way of creating
       *        an `Evaluator` with a `ParameterList`; however, it is
       *        _strongly_ advised that you _not_ use this `ParameterList`
       *        Constructor, but rather that you favor the main constructor
       *        with its compile-time argument checking instead.
       *
       *  \param[in] p A `ParameterList` of the form
                       \code{.xml}
                       <ParameterList>
                         <Parameter name = "Residual Names"     type = "std::vector<std::string>"            value = (required)    />
                         <Parameter name = "Vector Name"        type = "std::string"                         value = (required)    />
                         <Parameter name = "Basis"              type = "RCP<panzer::BasisIRLayout>"          value = (required)    />
                         <Parameter name = "IR"                 type = "RCP<panzer::IntegrationRule>"        value = (required)    />
                         <Parameter name = "Multiplier"         type = "double"                              value = (required)    />
                         <Parameter name = "Field Multipliers"  type = "RCP<const std::vector<std::string>>" value = null (default)/>
                         <Parameter name = "Data Layout Vector" type = "RCP<PHX::DataLayout>"                value = null (default)/>
                       </ParameterList>
                       \endcode
       *               where
       *               - "Residual Names" are the names for the terms this
       *                 `Evaluator` is evaluating,
       *               - "Vector Name" is the name of the vector-valued
       *                 function being integrated (\f$ \vec{s} \f$),
       *               - "Basis" is the basis that you'd like to use (\f$ \phi
       *                 \f$),
       *               - "IR" is the integration rule that you'd like to use,
       *               - "Multiplier" is the scalar multiplier out in front of
       *                 the integral you're computing (\f$ M \f$), and
       *               - "Field Multipliers" is an optional list of names of
       *                 fields that are multipliers out in front of the
       *                 integral you're computing (\f$ a(x) \f$, \f$ b(x) \f$,
       *                 etc.).
       *               - "Data Layout Vector" is the vector data layout that
       *                 you'd like to use.
       */
      Integrator_GradBasisCrossVector(
        const Teuchos::ParameterList& p);

      /**
       *  \brief Post-Registration Setup.
       *
       *  Sets the basis index, and gets the PHX::View versions of the field
       *  multipliers, if there are any.
       *
       *  \param[in] sd Essentially a list of `Workset`s, which are collections
       *                of cells (elements) that all live on a single process.
       *  \param[in] fm This is an unused part of the `Evaluator` interface.
       */
      void
      postRegistrationSetup(
        typename Traits::SetupData d,
        PHX::FieldManager<Traits>& fm);

      /**
       *  \brief Evaluate Fields.
       *
       *  This actually performs the integration by calling `operator()()` in a
       *  `Kokkos::parallel_for` over the cells in the `Workset`.
       *
       *  \param[in] workset The `Workset` on which you're going to do the
       *                     integration.
       */
      void
      evaluateFields(
        typename Traits::EvalData d);

      /**
       *  \brief This empty struct allows us to optimize `operator()()`
       *         depending on the number of field multipliers.
       */
      template<int NUM_FIELD_MULT>
      struct FieldMultTag
      {
      }; // end of struct FieldMultTag

      /**
       *  \brief Perform the integration.
       *
       *  Generally speaking, for a given cell in the `Workset`, this routine
       *  loops over quadrature points and bases to perform the integration,
       *  scaling the vector field to be integrated by the multiplier (\f$ M
       *  \f$) and any field multipliers (\f$ a(x) \f$, \f$ b(x) \f$, etc.).
       *
       *  \note Optimizations are made for the cases in which we have no field
       *        multipliers or only a single one.
       *
       *  \param[in] tag  An indication of the number of field multipliers we
       *                  have; either 0, 1, or something else.
       *  \param[in] cell The cell in the `Workset` over which to integrate.
       */
      template<int NUM_FIELD_MULT>
      KOKKOS_INLINE_FUNCTION
      void
      operator()(
        const FieldMultTag<NUM_FIELD_MULT>& tag,
        const std::size_t&                  cell) const;

    private:

      /**
       *  \brief Get Valid Parameters.
       *
       *  Get all the parameters that we support such that the `ParameterList`
       *  Constructor can do some validation of the input `ParameterList`.
       *
       *  \returns A `ParameterList` with all the valid parameters (keys) in
       *           it.  The values tied to those keys are meaningless default
       *           values.
       */
      Teuchos::RCP<Teuchos::ParameterList>
      getValidParameters() const;

      /**
       *  \brief The scalar type.
       */
      using ScalarT = typename EvalT::ScalarT;

      /**
       *  \brief An `enum` determining the behavior of this `Evaluator`.
       *
       *  This `Evaluator` will compute the result of its integration and then:
       *  - CONTRIBUTES:  contribute it to a specified residual, not saving
       *                  anything; or
       *  - EVALUATES:    save it under a specified name for future use.
       */
      const panzer::EvaluatorStyle evalStyle_;

      /**
       *  \brief The fields to which we'll contribute, or in which we'll store,
       *         the result of computing this integral.
       */
      // PHX::View<PHX::View<ScalarT**>*> fields_;
      using InnerView = PHX::View<ScalarT**>;
      using OuterView = PHX::View<InnerView*>;
      typename OuterView::HostMirror fields_host_;
      OuterView             fields_;

      /**
       *  \brief A field representing the vector-valued function we're
       *         integrating (\f$ \vec{s} \f$).
       */
      PHX::MDField<const ScalarT, Cell, IP, Dim> vector_;

      /**
       *  \brief The scalar multiplier out in front of the integral (\f$ M
       *         \f$).
       */
      double multiplier_;

      /**
       *  \brief The (possibly empty) list of fields that are multipliers out
       *         in front of the integral (\f$ a(x) \f$, \f$ b(x) \f$, etc.).
       */
      std::vector<PHX::MDField<const ScalarT, Cell, IP>> fieldMults_;

      /**
       *  \brief The `PHX::View` representation of the (possibly empty) list
       *         of fields that are multipliers out in front of the integral
       *         (\f$ a(x) \f$, \f$ b(x) \f$, etc.).
       */
      PHX::View<PHX::View<const ScalarT**>*> kokkosFieldMults_;

      /**
       *  \brief The number of dimensions associated with the vector.
       */
      int numDims_;

      /**
       *  \brief The number of dimensions associated with the gradient.
       */
      int numGradDims_;

      /**
       *  \brief The name of the basis we're using.
       */
      std::string basisName_;

      /**
       *  \brief The index in the `Workset` bases for our particular
       *         `BasisIRLayout` name.
       */
      std::size_t basisIndex_;

      /**
       *  \brief The gradient vector basis information necessary for
       *         integration.
       */
      PHX::MDField<double, panzer::Cell, panzer::BASIS, panzer::IP,
        panzer::Dim> basis_;

  }; // end of class Integrator_GradBasisCrossVector

} // end of namespace panzer

#endif // PANZER_EVALUATOR_GRADBASISCROSSVECTOR_DECL_HPP
