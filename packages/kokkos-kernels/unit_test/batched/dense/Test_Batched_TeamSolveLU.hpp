/// \author Vinh Dang (vqdang@sandia.gov)

#include "gtest/gtest.h"
#include "Kokkos_Core.hpp"
#include "Kokkos_Random.hpp"

//#include "KokkosBatched_Vector.hpp"

#include "KokkosBatched_Gemm_Decl.hpp"
#include "KokkosBatched_Gemm_Team_Impl.hpp"
#include "KokkosBatched_LU_Decl.hpp"
#include "KokkosBatched_LU_Team_Impl.hpp"
#include "KokkosBatched_SolveLU_Decl.hpp"
//#include "KokkosBatched_SolveLU_Team_Impl.hpp"

#include "KokkosKernels_TestUtils.hpp"

using namespace KokkosBatched;

namespace Test {
namespace TeamSolveLU {
	
  template<typename TA, typename TB>
  struct ParamTag { 
    typedef TA transA;
    typedef TB transB;
  };
 
  template<typename DeviceType,
           typename ViewType,
           typename ScalarType,
           typename ParamTagType, 
           typename AlgoTagType>
  struct Functor_BatchedTeamGemm {
    ViewType _a, _b, _c;
    
    ScalarType _alpha, _beta;
    
    KOKKOS_INLINE_FUNCTION
    Functor_BatchedTeamGemm(const ScalarType alpha, 
                            const ViewType &a,
                            const ViewType &b,
                            const ScalarType beta,
                            const ViewType &c)
      : _a(a), _b(b), _c(c), _alpha(alpha), _beta(beta) {}

    template<typename MemberType>
    KOKKOS_INLINE_FUNCTION
    void operator()(const ParamTagType &, const MemberType &member) const {
      const int k = member.league_rank();

      auto aa = Kokkos::subview(_a, k, Kokkos::ALL(), Kokkos::ALL());
      auto bb = Kokkos::subview(_b, k, Kokkos::ALL(), Kokkos::ALL());
      auto cc = Kokkos::subview(_c, k, Kokkos::ALL(), Kokkos::ALL());

      if (member.team_rank() == 0) {
        for (int i=0;i<static_cast<int>(aa.extent(0));++i)                                                                          
          aa(i,i) += 10.0;  
      }
      member.team_barrier();
	  
      KokkosBatched::TeamGemm<MemberType,
               typename ParamTagType::transA,
               typename ParamTagType::transB,
               AlgoTagType>::
        invoke(member, _alpha, aa, bb, _beta, cc);
    }
    
    inline
    void run() {
      typedef typename ViewType::value_type value_type;
      std::string name_region("KokkosBatched::Test::TeamSolveLU");
      std::string name_value_type = ( std::is_same<value_type,float>::value ? "::Float" : 
                                      std::is_same<value_type,double>::value ? "::Double" :
                                      std::is_same<value_type,Kokkos::complex<float> >::value ? "::ComplexFloat" :
                                      std::is_same<value_type,Kokkos::complex<double> >::value ? "::ComplexDouble" : "::UnknownValueType" );                               
      std::string name = name_region + name_value_type;
      Kokkos::Profiling::pushRegion( name.c_str() );
      const int league_size = _c.extent(0);
      Kokkos::TeamPolicy<DeviceType,ParamTagType> policy(league_size, Kokkos::AUTO);
      Kokkos::parallel_for((name+"::GemmFunctor").c_str(), policy, *this);            
      Kokkos::Profiling::popRegion(); 
    }
  };
  template<typename DeviceType,
           typename ViewType,
           typename AlgoTagType>
  struct Functor_BatchedTeamLU {
    ViewType _a;
    
    KOKKOS_INLINE_FUNCTION
    Functor_BatchedTeamLU(const ViewType &a) 
      : _a(a) {} 
    
    template<typename MemberType>
    KOKKOS_INLINE_FUNCTION
    void operator()(const MemberType &member) const {
      const int k = member.league_rank();
      auto aa = Kokkos::subview(_a, k, Kokkos::ALL(), Kokkos::ALL());
      
      if (member.team_rank() == 0) {
        for (int i=0;i<static_cast<int>(aa.extent(0));++i)                                                                          
          aa(i,i) += 10.0;  
      }
      member.team_barrier();
      
      KokkosBatched::TeamLU<MemberType,AlgoTagType>::invoke(member, aa);
    }
    inline
    void run() {
      typedef typename ViewType::value_type value_type;
      std::string name_region("KokkosBatched::Test::TeamSolveLU");
      std::string name_value_type = ( std::is_same<value_type,float>::value ? "::Float" : 
                                      std::is_same<value_type,double>::value ? "::Double" :
                                      std::is_same<value_type,Kokkos::complex<float> >::value ? "::ComplexFloat" :
                                      std::is_same<value_type,Kokkos::complex<double> >::value ? "::ComplexDouble" : "::UnknownValueType" );                               
      std::string name = name_region + name_value_type;
      Kokkos::Profiling::pushRegion( name.c_str() );
      const int league_size = _a.extent(0);
      Kokkos::TeamPolicy<DeviceType> policy(league_size, Kokkos::AUTO);
      Kokkos::parallel_for((name+"::LUFunctor").c_str(), policy, *this);
      Kokkos::Profiling::popRegion(); 
    }
  };
  template<typename DeviceType,
           typename ViewType,
           typename TransType,
           typename AlgoTagType>
  struct Functor_TestBatchedTeamSolveLU {
    ViewType _a;
    ViewType _b;
    
    KOKKOS_INLINE_FUNCTION
    Functor_TestBatchedTeamSolveLU(const ViewType &a, const ViewType &b) 
      : _a(a), _b(b) {} 
    
    template<typename MemberType>
    KOKKOS_INLINE_FUNCTION
    void operator()(const MemberType &member) const {
      const int k = member.league_rank();
      auto aa = Kokkos::subview(_a, k, Kokkos::ALL(), Kokkos::ALL());
      auto bb = Kokkos::subview(_b, k, Kokkos::ALL(), Kokkos::ALL());
      
      KokkosBatched::TeamSolveLU<MemberType,TransType,AlgoTagType>::invoke(member, aa, bb);
    }
    
    inline
    void run() {
      typedef typename ViewType::value_type value_type;
      std::string name_region("KokkosBatched::Test::TeamSolveLU");
      std::string name_value_type = ( std::is_same<value_type,float>::value ? "::Float" : 
                                      std::is_same<value_type,double>::value ? "::Double" :
                                      std::is_same<value_type,Kokkos::complex<float> >::value ? "::ComplexFloat" :
                                      std::is_same<value_type,Kokkos::complex<double> >::value ? "::ComplexDouble" : "::UnknownValueType" );                               
      std::string name = name_region + name_value_type;
      Kokkos::Profiling::pushRegion( name.c_str() );
      
      const int league_size = _a.extent(0);
      Kokkos::TeamPolicy<DeviceType> policy(league_size, Kokkos::AUTO);
      Kokkos::parallel_for((name+"::SolveLU").c_str(), policy, *this);
      Kokkos::Profiling::popRegion(); 
    }
  };
  
  template<typename DeviceType,
           typename ViewType,
           typename AlgoTagType>
  void impl_test_batched_solvelu(const int N, const int BlkSize) {
    typedef typename ViewType::value_type value_type;
    typedef Kokkos::Details::ArithTraits<value_type> ats;
    
    /// randomized input testing views
    ViewType a0("a0", N, BlkSize, BlkSize);
    ViewType a1("a1", N, BlkSize, BlkSize);
    ViewType b ("b",  N, BlkSize, 5 );
    ViewType x0("x0", N, BlkSize, 5 );
    //ViewType a0_T("a0_T", N, BlkSize, BlkSize);
    //ViewType b_T ("b_T",  N, BlkSize, 5 );
    
    Kokkos::Random_XorShift64_Pool<typename DeviceType::execution_space> random(13718);
    Kokkos::fill_random(a0, random, value_type(1.0));
    Kokkos::fill_random(x0, random, value_type(1.0));
    
    Kokkos::fence();
    
    Kokkos::deep_copy(a1, a0);
    //Kokkos::deep_copy(a0_T, a0);
    
    value_type alpha = 1.0, beta = 0.0;   
    typedef ParamTag<Trans::NoTranspose,Trans::NoTranspose> param_tag_type;
    
    Functor_BatchedTeamGemm<DeviceType,ViewType,value_type,
                            param_tag_type,AlgoTagType>(alpha, a0, x0, beta, b).run();
    
    Functor_BatchedTeamLU<DeviceType,ViewType,AlgoTagType>(a1).run();
    
    Functor_TestBatchedTeamSolveLU<DeviceType,ViewType,Trans::NoTranspose,AlgoTagType>(a1,b).run();
    
    Kokkos::fence();
    
    // //Transpose
    // typedef ParamTag<Trans::Transpose,Trans::NoTranspose> param_tag_type_T;
    
    // Functor_BatchedTeamGemm<DeviceType,ViewType,value_type,
    //   param_tag_type_T,AlgoTagType>(alpha, a0_T, x0, beta, b_T).run();
    
    // Functor_TestBatchedTeamSolveLU<DeviceType,ViewType,AlgoTagType,Trans::Transpose>(a1,b_T).run();
    
    // Kokkos::fence();
    
    /// for comparison send it to host
    typename ViewType::HostMirror x0_host = Kokkos::create_mirror_view(x0);
    typename ViewType::HostMirror b_host  = Kokkos::create_mirror_view(b);
    //typename ViewType::HostMirror b_T_host = Kokkos::create_mirror_view(b_T);
    
    Kokkos::deep_copy(x0_host, x0);
    Kokkos::deep_copy(b_host, b);
    //Kokkos::deep_copy(b_T_host, b_T);
    
    /// check x0 = b ; this eps is about 10^-14
    typedef typename ats::mag_type mag_type;
    mag_type sum(1), diff(0);
    const mag_type eps = 1.0e3 * ats::epsilon();
    
    for (int k=0;k<N;++k)
      for (int i=0;i<BlkSize;++i)
        for (int j=0;j<5;++j) {
          sum  += ats::abs(x0_host(k,i,j));
          diff += ats::abs(x0_host(k,i,j)-b_host(k,i,j));
        }
    //printf("NoTranspose -- N=%d, BlkSize=%d, sum=%f, diff=%f\n", N, BlkSize, sum, diff);
    EXPECT_NEAR_KK( diff/sum, 0.0, eps);
    
    // /// check x0 = b_T ; this eps is about 10^-14
    // mag_type sum_T(1), diff_T(0);
    
    // for (int k=0;k<N;++k)
    //   for (int i=0;i<BlkSize;++i)
    //     for (int j=0;j<5;++j) {
    //       sum_T  += ats::abs(x0_host(k,i,j));
    //       diff_T += ats::abs(x0_host(k,i,j)-b_T_host(k,i,j));
    //     }
    // //printf("Transpose -- N=%d, BlkSize=%d, sum=%f, diff=%f\n", N, BlkSize, sum_T, diff_T);
    // EXPECT_NEAR_KK( diff_T/sum_T, 0.0, eps);
  }
}
}


template<typename DeviceType,
         typename ValueType,
         typename AlgoTagType>
int test_batched_team_solvelu() {
#if defined(KOKKOSKERNELS_INST_LAYOUTLEFT)
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutLeft,DeviceType> ViewType;
    Test::TeamSolveLU::impl_test_batched_solvelu<DeviceType,ViewType,AlgoTagType>(     0, 10);
    for (int i=0;i<10;++i) {                                                                                         
      Test::TeamSolveLU::impl_test_batched_solvelu<DeviceType,ViewType,AlgoTagType>(1024,  i);
    }
  }
#endif
#if defined(KOKKOSKERNELS_INST_LAYOUTRIGHT)
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutRight,DeviceType> ViewType;
    Test::TeamSolveLU::impl_test_batched_solvelu<DeviceType,ViewType,AlgoTagType>(     0, 10);
    for (int i=0;i<10;++i) {                                                                                        
      Test::TeamSolveLU::impl_test_batched_solvelu<DeviceType,ViewType,AlgoTagType>(1024,  i);
    }
  }
#endif

  return 0;
}
