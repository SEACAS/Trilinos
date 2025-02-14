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

#ifndef   PANZER_EVALUATOR_GRADBASISTIMESSCALAR_IMPL_HPP
#define   PANZER_EVALUATOR_GRADBASISTIMESSCALAR_IMPL_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  Include Files
//
///////////////////////////////////////////////////////////////////////////////

// Intrepid2
#include "Intrepid2_FunctionSpaceTools.hpp"

// Kokkos
#include "Kokkos_ViewFactory.hpp"

// Panzer
#include "Panzer_BasisIRLayout.hpp"
#include "Panzer_IntegrationRule.hpp"
#include "Panzer_Workset_Utilities.hpp"

namespace panzer
{
  /////////////////////////////////////////////////////////////////////////////
  //
  //  Constructor
  //
  /////////////////////////////////////////////////////////////////////////////
  template<typename EvalT, typename Traits>
  Integrator_GradBasisTimesScalar<EvalT, Traits>::
  Integrator_GradBasisTimesScalar(
    const panzer::EvaluatorStyle&   evalStyle,
    const std::vector<std::string>& resNames,
    const std::string&              scalar,
    const panzer::BasisIRLayout&    basis,
    const panzer::IntegrationRule&  ir,
    const double&                   multiplier, /* = 1 */
    const std::vector<std::string>& fmNames)    /* =
      std::vector<std::string>() */
    :
    evalStyle_(evalStyle),
    multiplier_(multiplier),
    numDims_(ir.dl_vector->extent(2)),
    basisName_(basis.name())
  {
    using PHX::View;
    using panzer::BASIS;
    using panzer::Cell;
    using panzer::EvaluatorStyle;
    using panzer::IP;
    using panzer::PureBasis;
    using PHX::Device;
    using PHX::DevLayout;
    using PHX::MDField;
    using std::invalid_argument;
    using std::logic_error;
    using std::string;
    using Teuchos::RCP;

    // Ensure the input makes sense.
    for (const auto& name : resNames)
      TEUCHOS_TEST_FOR_EXCEPTION(name == "", invalid_argument, "Error:  "     \
        "Integrator_GradBasisTimesScalar called with an empty residual name.")
    TEUCHOS_TEST_FOR_EXCEPTION(scalar == "", invalid_argument, "Error:  "     \
      "Integrator_GradBasisTimesScalar called with an empty scalar name.")
    TEUCHOS_TEST_FOR_EXCEPTION(numDims_ != static_cast<int>(resNames.size()),
      logic_error, "Error:  Integrator_GradBasisTimesScalar:  The number of " \
      "residuals must equal the number of gradient components (mesh "         \
      "dimensions).")
    RCP<const PureBasis> tmpBasis = basis.getBasis();
    TEUCHOS_TEST_FOR_EXCEPTION(not tmpBasis->supportsGrad(), logic_error,
      "Error:  Integrator_GradBasisTimesScalar:  Basis of type \""
      << tmpBasis->name() << "\" does not support the gradient operation.")

    // Create the field or the scalar function we're integrating.
    scalar_ = MDField<const ScalarT, Cell, IP>(scalar, ir.dl_scalar);
    this->addDependentField(scalar_);

    // Create the fields that we're either contributing to or evaluating
    // (storing).
    fields_host_ = typename OuterView::HostMirror("Integrator_GradBasisCrossVector::fields_", resNames.size());
    fields_ = OuterView("Integrator_GradBasisCrossVector::fields_", resNames.size());
    {
      int i=0;
      for (const auto& name : resNames)
        fields_host_(i++) = PHX::View<ScalarT**>(name, basis.functional->extent(0),basis.functional->extent(1));
    }
    for (std::size_t i=0; i<fields_.extent(0); ++i) {
      const auto& field = fields_host_(i);
      PHX::Tag<ScalarT> tag(resNames[i],basis.functional);
      if (evalStyle_ == EvaluatorStyle::CONTRIBUTES)
        this->addContributedField(tag,field);
      else // if (evalStyle_ == EvaluatorStyle::EVALUATES)
        this->addEvaluatedField(tag,field);
    }

    // Add the dependent field multipliers, if there are any.
    int i = 0;
    fieldMults_.resize(fmNames.size());
    kokkosFieldMults_ = View<View<const ScalarT**>*>(
      "GradBasisTimesScalar::KokkosFieldMultipliers", fmNames.size());
    for (const auto& name : fmNames)
    {
      fieldMults_[i++] = MDField<const ScalarT, Cell, IP>(name, ir.dl_scalar);
      this->addDependentField(fieldMults_[i - 1]);
    } // end loop over the field multipliers

    // Set the name of this object.
    string n("Integrator_GradBasisTimesScalar (");
    if (evalStyle_ == EvaluatorStyle::CONTRIBUTES)
      n += "CONTRIBUTES";
    else // if (evalStyle_ == EvaluatorStyle::EVALUATES)
      n += "EVALUATES";
    n += "):  {";
    for (size_t j=0; j < fields_host_.extent(0) - 1; ++j)
      n += resNames[j] + ", ";
    n += resNames[resNames.size()-1] + "}";
    this->setName(n);
  } // end of Constructor

  /////////////////////////////////////////////////////////////////////////////
  //
  //  ParameterList Constructor
  //
  /////////////////////////////////////////////////////////////////////////////
  template<typename EvalT, typename Traits>
  Integrator_GradBasisTimesScalar<EvalT, Traits>::
  Integrator_GradBasisTimesScalar(
    const Teuchos::ParameterList& p)
    :
    Integrator_GradBasisTimesScalar(
      panzer::EvaluatorStyle::EVALUATES,
      p.get<const std::vector<std::string>>("Residual Names"),
      p.get<std::string>("Scalar Name"),
      (*p.get<Teuchos::RCP<panzer::BasisIRLayout>>("Basis")),
      (*p.get<Teuchos::RCP<panzer::IntegrationRule>>("IR")),
      p.get<double>("Multiplier"),
      p.isType<Teuchos::RCP<const std::vector<std::string>>>
        ("Field Multipliers") ?
        (*p.get<Teuchos::RCP<const std::vector<std::string>>>
        ("Field Multipliers")) : std::vector<std::string>())
  {
    using Teuchos::ParameterList;
    using Teuchos::RCP;

    // Ensure that the input ParameterList didn't contain any bogus entries.
    RCP<ParameterList> validParams = this->getValidParameters();
    p.validateParameters(*validParams);
  } // end of ParameterList Constructor

  /////////////////////////////////////////////////////////////////////////////
  //
  //  postRegistrationSetup()
  //
  /////////////////////////////////////////////////////////////////////////////
  template<typename EvalT, typename Traits>
  void
  Integrator_GradBasisTimesScalar<EvalT, Traits>::
  postRegistrationSetup(
    typename Traits::SetupData sd,
    PHX::FieldManager<Traits>& /* fm */)
  {
    using Kokkos::createDynRankView;
    using panzer::getBasisIndex;

    // Get the PHX::Views of the field multipliers.
    for (size_t i(0); i < fieldMults_.size(); ++i)
      kokkosFieldMults_(i) = fieldMults_[i].get_static_view();

    // Determine the index in the Workset bases for our particular basis name.
    basisIndex_ = getBasisIndex(basisName_, (*sd.worksets_)[0], this->wda);
  } // end of postRegistrationSetup()

  template<typename EvalT, typename Traits>
  template<int NUM_FIELD_MULT>
  KOKKOS_INLINE_FUNCTION
  void
  Integrator_GradBasisTimesScalar<EvalT, Traits>::
  operator()(
    const FieldMultTag<NUM_FIELD_MULT>& /* tag */,
    const size_t&                       cell) const
  {
    using panzer::EvaluatorStyle;

    // Initialize the evaluated fields.
    const int numQP(scalar_.extent(1)), numBases(fields_[0].extent(1));
    if (evalStyle_ == EvaluatorStyle::EVALUATES)
      for (int dim(0); dim < numDims_; ++dim)
        for (int basis(0); basis < numBases; ++basis)
          fields_[dim](cell, basis) = 0.0;

    // The following if-block is for the sake of optimization depending on the
    // number of field multipliers.
    ScalarT tmp;
    if (NUM_FIELD_MULT == 0)
    {
      // Loop over the quadrature points, scale the integrand by the
      // multiplier, and then perform the actual integration, looping over the
      // bases and dimensions.
      for (int qp(0); qp < numQP; ++qp)
      {
        tmp = multiplier_ * scalar_(cell, qp);
        for (int basis(0); basis < numBases; ++basis)
          for (int dim(0); dim < numDims_; ++dim)
            fields_[dim](cell, basis) += basis_(cell, basis, qp, dim) * tmp;
      } // end loop over the quadrature points
    }
    else if (NUM_FIELD_MULT == 1)
    {
      // Loop over the quadrature points, scale the integrand by the multiplier
      // and the single field multiplier, and then perform the actual
      // integration, looping over the bases and dimensions.
      for (int qp(0); qp < numQP; ++qp)
      {
        tmp = multiplier_ * scalar_(cell, qp) * kokkosFieldMults_(0)(cell, qp);
        for (int basis(0); basis < numBases; ++basis)
          for (int dim(0); dim < numDims_; ++dim)
            fields_[dim](cell, basis) += basis_(cell, basis, qp, dim) * tmp;
      } // end loop over the quadrature points
    }
    else
    {
      // Loop over the quadrature points and pre-multiply all the field
      // multipliers together, scale the integrand by the multiplier and
      // the combination of the field multipliers, and then perform the actual
      // integration, looping over the bases and dimensions.
      const int numFieldMults(kokkosFieldMults_.extent(0));
      for (int qp(0); qp < numQP; ++qp)
      {
        ScalarT fieldMultsTotal(1);
        for (int fm(0); fm < numFieldMults; ++fm)
          fieldMultsTotal *= kokkosFieldMults_(fm)(cell, qp);
        tmp = multiplier_ * scalar_(cell, qp) * fieldMultsTotal;
        for (int basis(0); basis < numBases; ++basis)
          for (int dim(0); dim < numDims_; ++dim)
            fields_[dim](cell, basis) += basis_(cell, basis, qp, dim) * tmp;
      } // end loop over the quadrature points
    } // end if (NUM_FIELD_MULT == something)
  } // end of operator()()

  /////////////////////////////////////////////////////////////////////////////
  //
  //  evaluateFields()
  //
  /////////////////////////////////////////////////////////////////////////////
  template<typename EvalT, typename Traits>
  void
  Integrator_GradBasisTimesScalar<EvalT, Traits>::
  evaluateFields(
    typename Traits::EvalData workset)
  {
    using Kokkos::parallel_for;
    using Kokkos::RangePolicy;
        
    // Grab the basis information.
    basis_ = this->wda(workset).bases[basisIndex_]->weighted_grad_basis;
            
    // The following if-block is for the sake of optimization depending on the
    // number of field multipliers.  The parallel_fors will loop over the cells
    // in the Workset and execute operator()() above.
    if (fieldMults_.size() == 0)
      parallel_for(RangePolicy<FieldMultTag<0>>(0, workset.num_cells), *this);
    else if (fieldMults_.size() == 1)
      parallel_for(RangePolicy<FieldMultTag<1>>(0, workset.num_cells), *this);
    else    
      parallel_for(RangePolicy<FieldMultTag<-1>>(0, workset.num_cells), *this);
  } // end of evaluateFields()

  /////////////////////////////////////////////////////////////////////////////
  //
  //  getValidParameters()
  //
  /////////////////////////////////////////////////////////////////////////////
  template<typename EvalT, typename TRAITS>
  Teuchos::RCP<Teuchos::ParameterList>
  Integrator_GradBasisTimesScalar<EvalT, TRAITS>::
  getValidParameters() const
  {
    using panzer::BasisIRLayout;
    using panzer::IntegrationRule;
    using std::string;
    using std::vector;
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    using Teuchos::rcp;

    // Create a ParameterList with all the valid keys we support.
    RCP<ParameterList> p = rcp(new ParameterList);

    RCP<const vector<string>> resNames;
    p->set("Residual Names", resNames);
    p->set<string>("Scalar Name", "?");
    RCP<BasisIRLayout> basis;
    p->set("Basis", basis);
    RCP<IntegrationRule> ir;
    p->set("IR", ir);
    p->set<double>("Multiplier", 1.0);
    RCP<const vector<string>> fms;
    p->set("Field Multipliers", fms);

    return p;
  } // end of getValidParameters()

} // end of namespace panzer

#endif // PANZER_EVALUATOR_GRADBASISTIMESSCALAR_IMPL_HPP
