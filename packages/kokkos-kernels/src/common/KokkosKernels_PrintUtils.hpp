/*
//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
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
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef _KOKKOSKERNELS_PRINTUTILS_HPP
#define _KOKKOSKERNELS_PRINTUTILS_HPP
#include "Kokkos_Core.hpp"
#include "Kokkos_Atomic.hpp"
#include "Kokkos_Timer.hpp"
#include <ostream>

namespace KokkosKernels{


namespace Impl{



template <typename in_lno_view_t,
          typename out_lno_view_t>
struct Histogram{
  in_lno_view_t inview;
  out_lno_view_t outview;
  Histogram (in_lno_view_t inview_, out_lno_view_t outview_): inview(inview_), outview(outview_){}

  KOKKOS_INLINE_FUNCTION
  void operator()(const size_t &ii) const {
    typedef typename std::remove_reference< decltype(outview(0)) >::type atomic_incr_type;
    Kokkos::atomic_fetch_add(&(outview(inview(ii))), atomic_incr_type(1));
  }
};


/**
 * \brief given an integer input view, it fills the histogram that
 * represents how many of each value exists.
 * \param in_elements: number of the elements in input view.
 * \param in_view: the input view. Has to be integer-like.
 * \param histogram: the output histogram. User is responsible from initializing them with 0, and size
 * must be big enough to hold all values in input view.
 */
template <typename in_lno_view_t,
          typename out_lno_view_t,
          typename MyExecSpace>
inline void kk_get_histogram(
    typename in_lno_view_t::size_type in_elements,
    in_lno_view_t in_view,
    out_lno_view_t histogram /*must be initialized with 0s*/){
  typedef Kokkos::RangePolicy<MyExecSpace> my_exec_space;
  Kokkos::parallel_for( "KokkosKernels::Common::GetHistogram", my_exec_space(0, in_elements), Histogram<in_lno_view_t, out_lno_view_t>(in_view, histogram));
  MyExecSpace().fence();
}


/**
 * \brief Prints the given 1D view.
 * \param os: Stream to print to. To print to stdout use std::cout, stderr, std::cerr, or a file use an ofstream object.
 * \param view: input view to print.
 * \param print_all: whether to print all elements or not. If it is false, print print_size/2 first and last elements.
 * \param sep: Element separator. Default is a single space: " "
 * \param print_size: Total elements to print if print_all is false print_size/2 first and last elements are pritned.
 *                    This parameter is not used if print_all is set to true.
 */
template <typename idx_array_type>
inline void kk_print_1Dview(std::ostream& os, idx_array_type view, bool print_all=false, const char* sep=" ", size_t print_size=40)
{
  typedef typename idx_array_type::HostMirror host_type;
  typedef typename idx_array_type::size_type idx;
  host_type host_view = Kokkos::create_mirror_view (view);
  Kokkos::deep_copy (host_view, view);
  idx nr = host_view.extent(0);
  if (!print_all)
  {
    if (nr > print_size)
    {
      idx n = print_size / 2;
      for (idx i = 0; i < n; ++i)
      {
        os << host_view(i) << sep;
      }
      os << "... ... ..." << sep;

      for (idx i = nr-n; i < nr; ++i)
      {
        os << host_view(i) << sep;
      }
      os << std::endl;
    }
    else
    {
      for (idx i = 0; i < nr; ++i)
      {
        os << host_view(i) << sep;
      }
      os << std::endl;
    }
  }
  else
  {
    for (idx i = 0; i < nr; ++i)
    {
      os << host_view(i) << sep;
    }
    os << std::endl;
  }
}


/**
 * \brief Prints the given 1D view.
 * \param view: input view to print.
 * \param print_all: whether to print all elements or not. If it is false,
 * only first and last 20 elements are printed.
 * 
 * This interface is provided for backwards compatiblity.
 */
template <typename idx_array_type>
inline void kk_print_1Dview(idx_array_type view, bool print_all = false, size_t print_size = 40){

  kk_print_1Dview(std::cout, view, print_all, " ", print_size);

}

}
}


#endif
