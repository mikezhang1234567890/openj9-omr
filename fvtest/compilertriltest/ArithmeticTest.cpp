/*******************************************************************************
 * Copyright (c) 2017, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "OpCodeTest.hpp"
#include "default_compiler.hpp"

#include <cmath>

int32_t iadd(int32_t l, int32_t r) {
    return l+r;
}

int32_t isub(int32_t l, int32_t r) {
    return l-r;
}

int32_t imul(int32_t l, int32_t r) {
    return l*r;
}

int32_t idiv(int32_t l, int32_t r) {
    return l/r;
}

int32_t irem(int32_t l, int32_t r) {
    return l%r;
}

int64_t ladd(int64_t l, int64_t r) {
    return l+r;
}

int64_t lsub(int64_t l, int64_t r) {
    return l-r;
}

class Int32Arithmetic : public TRTest::BinaryOpTest<int32_t> {};

class Int64Arithmetic : public TRTest::BinaryOpTest<int64_t> {};

TEST_P(Int32Arithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 (block (ireturn (%s (iconst %d) (iconst %d)) )))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int32_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int32Arithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 args=[Int32, Int32] (block (ireturn (%s (iload parm=0) (iload parm=1)) )))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t, int32_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

TEST_P(Int64Arithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int64 (block (lreturn (%s (lconst %lld) (lconst %lld)) )))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int64_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int64Arithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int64 args=[Int64, Int64] (block (lreturn (%s (lload parm=0) (lload parm=1)) )))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int64_t (*)(int64_t, int64_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

INSTANTIATE_TEST_CASE_P(ArithmeticTest, Int32Arithmetic, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_value_pairs<int32_t, int32_t>()),
    ::testing::Values(
        std::make_tuple("iadd", iadd),
        std::make_tuple("isub", isub),
        std::make_tuple("imul", imul) )));

INSTANTIATE_TEST_CASE_P(ArithmeticTest, Int64Arithmetic, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_value_pairs<int64_t, int64_t>()),
    ::testing::Values(
        std::make_tuple("ladd", ladd),
        std::make_tuple("lsub", lsub) )));

/**
 * @brief Filter function for *div opcodes
 * @tparam T is the data type of the div opcode
 * @param a is the set of input values to be tested for filtering
 *
 * This function is a predicate for filtering invalid input values for the
 * *div opcodes. This includes pairs where the divisor is 0, and pairs where
 * dividend is the minimum (negative) value for the type and the divisor is -1.
 * The latter is a problem because in two's complement, the quotient of the
 * signed minimum and -1 is greater than the maximum value representable by
 * the given type. On some platforms this computation results in SIGFPE.
 */
template <typename T>
bool div_filter(std::tuple<T, T> a)
   {
   auto isDivisorZero = std::get<1>(a) == 0;
   auto isDividendMin = std::get<0>(a) == std::numeric_limits<T>::min();
   auto isDivisorNegativeOne = std::get<1>(a) == TRTest::negative_one_value<T>();

   return isDivisorZero || (isDividendMin && isDivisorNegativeOne);
   }

INSTANTIATE_TEST_CASE_P(DivArithmeticTest, Int32Arithmetic, ::testing::Combine(
    ::testing::ValuesIn(
        TRTest::filter(TRTest::const_value_pairs<int32_t, int32_t>(), div_filter<int32_t> )),
    ::testing::Values(
        std::make_tuple("idiv", idiv),
        std::make_tuple("irem", irem) )));

template <typename T>
bool smallFp_filter(std::tuple<T, T> a)
   {
   // workaround: avoid failure caused by snprintf("%f")
   auto a0 = std::get<0>(a);
   auto a1 = std::get<1>(a);
   return ((abs(a0) < 0.01 && a0 != 0.0) || (abs(a1) < 0.01 && a1 != 0.0));
   }

float fadd(float l, float r) {
    return l+r;
}

float fsub(float l, float r) {
    return l-r;
}

float fmul(float l, float r) {
    return l*r;
}

float fdiv(float l, float r) {
    return l/r;
}

class FloatArithmetic : public TRTest::BinaryOpTest<float> {};

TEST_P(FloatArithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Float (block (freturn (%s (fconst %f) (fconst %f)) )))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<float (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

TEST_P(FloatArithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Float args=[Float, Float] (block (freturn (%s (fload parm=0) (fload parm=1)) )))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<float (*)(float, float)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point(param.lhs, param.rhs);
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

INSTANTIATE_TEST_CASE_P(ArithmeticTest, FloatArithmetic, ::testing::Combine(
    ::testing::ValuesIn(
        TRTest::filter(TRTest::const_value_pairs<float, float>(), smallFp_filter<float>)),
    ::testing::Values(
        std::make_tuple("fadd", fadd),
        std::make_tuple("fsub", fsub),
        std::make_tuple("fmul", fmul),
        std::make_tuple("fdiv", fdiv)
    )));

double dadd(double l, double r) {
    return l+r;
}

double dsub(double l, double r) {
    return l-r;
}

double dmul(double l, double r) {
    return l*r;
}

double ddiv(double l, double r) {
    return l/r;
}

class DoubleArithmetic : public TRTest::BinaryOpTest<double> {};

TEST_P(DoubleArithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[1024] = {0};
    std::snprintf(inputTrees, 1024, "(method return=Double (block (dreturn (%s (dconst %f) (dconst %f)) )))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<double (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

TEST_P(DoubleArithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Double args=[Double, Double] (block (dreturn (%s (dload parm=0) (dload parm=1)) )))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<double (*)(double, double)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point(param.lhs, param.rhs);
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

INSTANTIATE_TEST_CASE_P(ArithmeticTest, DoubleArithmetic, ::testing::Combine(
    ::testing::ValuesIn(
        TRTest::filter(TRTest::const_value_pairs<double, double>(), smallFp_filter<double>)),
    ::testing::Values(
        std::make_tuple("dadd", dadd),
        std::make_tuple("dsub", dsub),
        std::make_tuple("dmul", dmul),
        std::make_tuple("ddiv", ddiv)
    )));

template <typename T>
bool smallFp_unary_filter(T a)
   {
   // workaround: avoid failure caused by snprintf("%f")
   return (std::abs(a) < 0.01 && a != 0.0);
   }

float fneg(float x) {
    return -x;
}

class FloatUnaryArithmetic : public TRTest::UnaryOpTest<float> {};

TEST_P(FloatUnaryArithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Float (block (freturn (%s (fconst %f)))))", param.opcode.c_str(), param.value);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<float (*)(void)>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point();
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

TEST_P(FloatUnaryArithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Float args=[Float] (block (freturn (%s (fload parm=0)))))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<float (*)(float)>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point(param.value);
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

INSTANTIATE_TEST_CASE_P(ArithmeticTest, FloatUnaryArithmetic, ::testing::Combine(
    ::testing::ValuesIn(
        TRTest::filter(TRTest::const_values<float>(), smallFp_unary_filter<float>)),
    ::testing::Values(
        std::make_tuple("fabs", static_cast<float (*)(float)>(std::abs)),
        std::make_tuple("fneg", fneg)
    )));

double dneg(double x) {
    return -x;
}

class DoubleUnaryArithmetic : public TRTest::UnaryOpTest<double> {};

TEST_P(DoubleUnaryArithmetic, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[512] = {0};
    std::snprintf(inputTrees, 512, "(method return=Double (block (dreturn (%s (dconst %f)))))", param.opcode.c_str(), param.value);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<double (*)(void)>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point();
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

TEST_P(DoubleUnaryArithmetic, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160, "(method return=Double args=[Double] (block (dreturn (%s (dload parm=0)))))", param.opcode.c_str());
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler{trees};

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<double (*)(double)>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point(param.value);
    if (std::isnan(exp)) {
        ASSERT_EQ(std::isnan(exp), std::isnan(act));
    } else {
        ASSERT_EQ(exp, act);
    }
}

INSTANTIATE_TEST_CASE_P(ArithmeticTest, DoubleUnaryArithmetic, ::testing::Combine(
    ::testing::ValuesIn(
        TRTest::filter(TRTest::const_values<double>(), smallFp_unary_filter<double>)),
    ::testing::Values(
        std::make_tuple("dabs", static_cast<double (*)(double)>(std::abs)),
        std::make_tuple("dneg", dneg)
    )));
