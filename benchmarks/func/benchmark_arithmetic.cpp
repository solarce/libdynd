//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

#include <benchmark/benchmark.h>

#include <dynd/func/random.hpp>

using namespace std;
using namespace dynd;

static const int size = 100000;

static void BM_Func_Arithmetic_Add(benchmark::State &state)
{
  nd::array a = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  nd::array b = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  while (state.KeepRunning()) {
    a + b;
  }
}

BENCHMARK(BM_Func_Arithmetic_Add);

#ifdef DYND_CUDA
static void BM_Func_Arithmetic_CUDADevice_Add(benchmark::State &state)
{
  nd::array a = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  a = a.to_cuda_device();
  nd::array b = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  b = b.to_cuda_device();
  while (state.KeepRunning()) {
    a + b;
  }
}

BENCHMARK(BM_Func_Arithmetic_CUDADevice_Add);

#endif

static void BM_Func_Arithmetic_Mul(benchmark::State &state)
{
  nd::array a = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  nd::array b = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  while (state.KeepRunning()) {
    a * b;
  }
}

BENCHMARK(BM_Func_Arithmetic_Mul);

#ifdef DYND_CUDA

static void BM_Func_Arithmetic_CUDADevice_Mul(benchmark::State &state)
{
  nd::array a = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  a = a.to_cuda_device();
  nd::array b = nd::random::uniform(kwds("dst_tp", ndt::make_fixed_dim(size, ndt::make_type<double>())));
  b = b.to_cuda_device();
  while (state.KeepRunning()) {
    a * b;
  }
}

BENCHMARK(BM_Func_Arithmetic_CUDADevice_Mul);

#endif