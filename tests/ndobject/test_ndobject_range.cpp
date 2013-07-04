//
// Copyright (C) 2011-13 Mark Wiebe, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <inc_gtest.hpp>

#include <dynd/ndobject.hpp>
#include <dynd/ndobject_range.hpp>

using namespace std;
using namespace dynd;

TEST(NDObjectRange, Basic) {
    nd::array a;

    a = nd::range(1, 10);
    EXPECT_EQ(make_dtype<int32_t>(), a.get_dtype().get_udtype());
    EXPECT_EQ(1u, a.get_shape().size());
    EXPECT_EQ(9, a.get_shape()[0]);
    for (int i = 0; i < 9; ++i) {
        EXPECT_EQ(i+1, a.at(i).as<int32_t>());
    }

    a = nd::range(1., 10., 0.5);
    EXPECT_EQ(make_dtype<double>(), a.get_dtype().get_udtype());
    EXPECT_EQ(1u, a.get_shape().size());
    EXPECT_EQ(18, a.get_shape()[0]);
    for (int i = 0; i < 18; ++i) {
        EXPECT_EQ(0.5*(i+2), a.at(i).as<double>());
    }

    a = nd::range(0., 1., 0.1);
    EXPECT_EQ(make_dtype<double>(), a.get_dtype().get_udtype());
    EXPECT_EQ(10, a.get_shape()[0]);

    a = nd::range(0.f,1.f,0.01f);
    EXPECT_EQ(make_dtype<float>(), a.get_dtype().get_udtype());
    EXPECT_EQ(100, a.get_shape()[0]);

    a = nd::range(3 <= irange() <= 20);
    for (int i = 3; i <= 20; ++i) {
        EXPECT_EQ(i, a.at(i-3).as<int32_t>());
    }
}

TEST(NDObjectRange, CastScalars) {
    nd::array a;

    a = nd::range(4).ucast(make_dtype<int32_t>());
    a = a.eval();
    EXPECT_EQ(0, a.at(0).as<int32_t>());
    EXPECT_EQ(1, a.at(1).as<int32_t>());
    EXPECT_EQ(2, a.at(2).as<int32_t>());
    EXPECT_EQ(3, a.at(3).as<int32_t>());
    a = a.ucast(make_dtype<double>());
    a = a.eval();
    EXPECT_EQ(0., a.at(0).as<double>());
    EXPECT_EQ(1., a.at(1).as<double>());
    EXPECT_EQ(2., a.at(2).as<double>());
    EXPECT_EQ(3., a.at(3).as<double>());
}

