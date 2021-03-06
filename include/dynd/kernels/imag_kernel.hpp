//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/kernels/base_kernel.hpp>

namespace dynd {
namespace nd {

  template <type_id_t RealTypeID>
  struct imag_kernel : base_kernel<imag_kernel<RealTypeID>, 1> {
    typedef typename type_of<RealTypeID>::type real_type;
    typedef complex<typename type_of<RealTypeID>::type> complex_type;

    void single(char *dst, char *const *src)
    {
      *reinterpret_cast<real_type *>(dst) = reinterpret_cast<complex_type *>(src[0])->imag();
    }
  };

} // namespace dynd::nd

namespace ndt {

  template <type_id_t RealTypeID>
  struct type::equivalent<nd::imag_kernel<RealTypeID>> {
    static type make()
    {
      return callable_type::make(type::make<typename nd::imag_kernel<RealTypeID>::real_type>(),
                                 {type::make<typename nd::imag_kernel<RealTypeID>::complex_type>()});
    }
  };

} // namespace dynd::ndt
} // namespace dynd
