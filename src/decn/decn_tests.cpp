// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*
 * decn_tests.cpp
 *
 * Unit tests using https://github.com/catchorg/Catch2
 *
 *  Created on: Nov 14, 2019
 */


#include <string>
#include <random>
#include <boost/multiprecision/mpfr.hpp>
#include <catch2/catch.hpp>
#include "decn.h"
#include "../utils.h"


namespace bmp = boost::multiprecision;
using Catch::Matchers::Equals;

static const int NUM_RAND_TESTS = 123456;


TEST_CASE("build decn"){
	build_dec80("0.0009234567890123456", 7);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("9234.567890123456"));

	build_dec80("9.234567890123456", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("9234.567890123456"));

	negate_decn(&AccDecn);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-9234.567890123456"));

	//small positive
	build_dec80(".1", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("100."));

	build_dec80("0.1", -1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0.01"));

	build_dec80("0.01", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("10."));

	//zero
	build_dec80(".", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	build_dec80(".0", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	build_dec80("0.", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	build_dec80("-0.0", -1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	build_dec80("0", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	//small negative
	build_dec80("-.1", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-100."));

	build_dec80("-0.1", -1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-0.01"));

	build_dec80("-0.001", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-1."));

	//empty string -> 0
	build_dec80("", 90);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));

	//too many .
	build_dec80("..", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	//very long (truncated)
	build_dec80("12345678901234567890", -2);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("123456789012345678."));

	build_dec80("12345678901234567890", +2);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("1.23456789012345678E21"));

	//overflow
	build_dec80("100", DEC80_MAX_EXP-1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	build_dec80("1", DEC80_MAX_EXP+1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	build_dec80("0.1", DEC80_MAX_EXP+2);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	//underflow
	build_dec80("10", DEC80_MIN_EXP-2);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	build_dec80("1", DEC80_MIN_EXP-1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	build_dec80("0.3", DEC80_MIN_EXP);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));
	CHECK(decn_is_nan(&AccDecn) == 1);

	//left/right count
	build_dec80("-100.001", 3);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-100001."));

	//invalid
	build_dec80(":", 0);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error"));

	//special number that is not NaN
	AccDecn.lsu[1] = 3;
	CHECK(decn_is_nan(&AccDecn) == 0);
}

TEST_CASE("build_large"){
	int large_exp = DEC80_MAX_EXP/2 - 50;
	build_dec80("9.99", large_exp);
	decn_to_str_complete(&AccDecn);
	CHECK(AccDecn.exponent == large_exp);
	std::string expected = "9.99E";
	expected += std::to_string(large_exp);
	CHECK_THAT(Buf, Equals(expected));
}

TEST_CASE("small fractions >= 1/10"){
	build_dec80("0.333", 0);
	build_decn_at(&BDecn,   "3.33", -1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0.333"));
	decn_to_str_complete(&BDecn);
	CHECK_THAT(Buf, Equals("0.333"));
	negate_decn(&BDecn);
	add_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("0"));
}

TEST_CASE("add"){
	dec80 tmp_copy;
	build_dec80("-9.234567890123456", 3);
	copy_decn(&tmp_copy, &AccDecn); //save
	build_dec80("-92.3456789012345678", 1);
	copy_decn(&BDecn, &AccDecn);
	copy_decn(&AccDecn, &tmp_copy); //restore
	copy_decn(&tmp_copy, &BDecn); //save
	decn_to_str_complete(&BDecn);
	CHECK_THAT(Buf, Equals("-923.456789012345678")); //b

	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-9234.567890123456")); //-acc

	//compare result of b - acc
	add_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-10158.0246791358016")); //b-acc

	//new acc for acc - b test
	decn_to_str_complete(&AccDecn);
	copy_decn(&BDecn, &tmp_copy); //restore
	negate_decn(&BDecn);
	decn_to_str_complete(&BDecn);
	CHECK_THAT(Buf, Equals("923.456789012345678")); //-b

	add_decn();
	decn_to_str_complete(&AccDecn);
	//compare result of new acc - b
	CHECK_THAT(Buf, Equals("-9234.567890123456")); //acc-b

	//add 0
	build_decn_at(&BDecn, "0", 0);
	add_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-9234.567890123456")); //same

	//carry into MSB
	build_dec80(          "-82345678901234567.8", -1);
	build_decn_at(&BDecn, "-87654321098765432.2", -1);
	add_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf,Equals("-17000000000000000.")); //acc+b

	//don't negate NaN
	set_dec80_NaN(&AccDecn);
	negate_decn(&AccDecn);
	CHECK(decn_is_nan(&AccDecn));
}

TEST_CASE("multiply"){
	build_dec80("92.34567890123456", 2);
	build_decn_at(&BDecn,   "-92.3456789012345678", 1);
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("9234.567890123456")); //acc
	decn_to_str_complete(&BDecn);
	CHECK_THAT(Buf, Equals("-923.456789012345678")); //b
	mult_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("-8527724.41172991849")); //acc*b

	//overflow
	build_dec80("9.99", DEC80_MAX_EXP/2);
	build_decn_at(&BDecn, "9.99", DEC80_MAX_EXP/2);
	mult_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error")); //acc*b

	//NaN
	build_dec80("9.99", DEC80_MAX_EXP/2);
	set_dec80_NaN(&BDecn);
	mult_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error")); //acc*b

	//NaN
	set_dec80_NaN(&AccDecn);
	build_decn_at(&BDecn, "9.99", DEC80_MAX_EXP/2);
	mult_decn();
	decn_to_str_complete(&AccDecn);
	CHECK_THAT(Buf, Equals("Error")); //acc*b
}

static void div_test(){ //acc / b
	bmp::mpf_float::default_precision(50);
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);
	bmp::mpfr_float a_actual(Buf);
	decn_to_str_complete(&BDecn);
	CAPTURE(Buf);
	bmp::mpfr_float b_actual(Buf);
	//calc result
	div_decn();
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);  // acc / b

	//calculate actual result
	a_actual /= b_actual;
	if (decn_is_nan(&AccDecn)){
		//check that NaN result of division by 0
		CAPTURE(a_actual);
		CHECK(b_actual == 0);
	} else {
		bmp::mpfr_float calculated(Buf);
		bmp::mpfr_float rel_diff = abs((a_actual - calculated) / a_actual);
		CHECK(rel_diff < 2e-17);
	}
}

static void div_test(
	//input
	const char* a_str, int a_exp,
	const char* b_str, int b_exp
)
{
	CAPTURE(a_str); CAPTURE(a_exp);
	CAPTURE(b_str); CAPTURE(b_exp);
	//do division
	build_dec80(a_str, a_exp);
	build_decn_at(&BDecn,   b_str, b_exp);
	div_test();
}

TEST_CASE("division"){
	div_test(
		"1", 0,
		"0", 0
	);

	div_test(
		"3.14", 60,
		"-1.5", -2
	);

	div_test(
		"4", 0,
		"4", 0
	);

	div_test(
		"1", 0,
		"3", 0
	);

	div_test(
		"500", 0,
		"99", 0
	);

	div_test(
		"500", 0,
		"2", 0
	);

	div_test(
		"3", 0,
		"25", -15
	);

	div_test(
		"0.02", 0,
		"0.03", 0
	);
}

TEST_CASE("division random"){
	std::default_random_engine gen;
	std::uniform_int_distribution<int> distrib(0, 99);
	std::uniform_int_distribution<int> sign_distrib(0,1);
	for (int j = 0; j < NUM_RAND_TESTS; j++){
		AccDecn.lsu[0] = distrib(gen);
		BDecn.lsu[0] = distrib(gen);
		for (int i = 1; i < DEC80_NUM_LSU; i++){
			AccDecn.lsu[i] = distrib(gen);
			BDecn.lsu[i] = distrib(gen);
		}
		set_exponent(&AccDecn, distrib(gen), sign_distrib(gen));
		set_exponent(&BDecn, distrib(gen), sign_distrib(gen));
		div_test();
	}
}

static void sqrt_test(){
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);
	//calculate result
	sqrt_decn();
	//build mpfr float
	bmp::mpfr_float::default_precision(50);
	bmp::mpfr_float x_actual(Buf);
	//print calc result
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);
	//calculate actual result
	CAPTURE(x_actual);
	if (decn_is_nan(&AccDecn)){
		//check that NaN is from result of sqrt(-)
		CHECK(x_actual <= 0);
	} else if (decn_is_zero(&AccDecn)){
		//check actual is also 0
		CHECK(x_actual == 0);
	} else {
		x_actual = sqrt(x_actual);
		CAPTURE(x_actual);
		bmp::mpfr_float calculated(Buf);
		bmp::mpfr_float rel_diff = abs((x_actual - calculated) / x_actual);
		CHECK(rel_diff < 2e-17);
	}
}

static void sqrt_test(const char* x_str, int x_exp)
{
	CAPTURE(x_str); CAPTURE(x_exp);
	build_dec80(x_str, x_exp);
	sqrt_test();
}

TEST_CASE("sqrt"){
	sqrt_test("0", 0);
	sqrt_test("2", 0);
	sqrt_test("-1", 0);
	sqrt_test("0.155", 0);
	sqrt_test("10", 0);
	sqrt_test("1.1", 10);
	sqrt_test("2.02", -10);
	sqrt_test("2.02", 0);
	sqrt_test("1.5", 0);
	sqrt_test("9", 99);
	sqrt_test("123", 12345);
}

TEST_CASE("sqrt random"){
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0,99);
	std::uniform_int_distribution<int> exp_distrib(-99,99);
	std::uniform_int_distribution<int> sign_distrib(0,1);
	for (int j = 0; j < NUM_RAND_TESTS; j++){
		for (int i = 0; i < DEC80_NUM_LSU; i++){
			AccDecn.lsu[i] = distribution(generator);
		}
		int sign = sign_distrib(generator);
		set_exponent(&AccDecn, exp_distrib(generator), sign);
		sqrt_test();
	}
}

static void log_test_(bool base10, double epsilon){
	bmp::mpfr_float::default_precision(50);
	CAPTURE(base10);
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);
	//build mpfr float
	bmp::mpfr_float x_actual(Buf);
	//calculate result
	if (base10){
		log10_decn();
	} else {
		ln_decn();
	}
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);  // log(x)

	//calculate actual result
	CAPTURE(x_actual);
	if (decn_is_nan(&AccDecn)){
		//check that NaN is from result of log(-)
		CHECK(x_actual <= 0);
	} else {
		if (base10){
			x_actual = log10(x_actual);
		} else {
			x_actual = log(x_actual);
		}
		bmp::mpfr_float calculated(Buf);
		CAPTURE(calculated);
		bmp::mpfr_float rel_diff = abs((x_actual - calculated) / x_actual);
		CHECK(rel_diff < epsilon);
	}
}

static void log_test(bool base10=false){
	//check if near 1.0
	remove_leading_zeros(&AccDecn);
	double lsu0 = AccDecn.lsu[0];
	int exp = get_exponent(&AccDecn);
	if (exp == -1){
		lsu0 /= (double) 10;
		lsu0 += (double) AccDecn.lsu[1] / (10*100);
		lsu0 += (double) AccDecn.lsu[2] / (10*100*100);
		lsu0 += (double) AccDecn.lsu[3] / (10*100*100*100);
	} else if (exp == 0){
		lsu0 += (double) AccDecn.lsu[1] / 100;
		lsu0 += (double) AccDecn.lsu[2] / (100*100);
		lsu0 += (double) AccDecn.lsu[3] / (100*100*100);
	}
	CAPTURE((int) AccDecn.lsu[0]); CAPTURE((int) AccDecn.lsu[1]);
	CAPTURE(exp);
	CAPTURE(lsu0);
	if (exp == 0 || exp == -1){
		//check if near 1.0
		if (lsu0 >= 7 && lsu0 < 8){
			log_test_(base10, 7.5e-16);
		} else if (lsu0 >= 8 && lsu0 < 9){
			log_test_(base10, 1.5e-15);
		} else if (lsu0 >= 9 && lsu0 < 9.6){
			log_test_(base10, 1.0e-14);
		} else if (lsu0 >= 9.6 && lsu0 < 9.9){
			log_test_(base10, 4.1e-13);
		} else if (lsu0 >= 9.9 && lsu0 < 9.999){
			log_test_(base10, 1.5e-11);
		} else if (lsu0 >= 9.999 && lsu0 < 9.99999){
			log_test_(base10, 6.0e-10);
		} else if (lsu0 >= 9.99999 && lsu0 < 9.9999999){
			log_test_(base10, 3.0e-9);
		} else if (lsu0 >= 9.9999999 && lsu0 < 10.0){
			log_test_(base10, 1.3e-7);
		} else if (lsu0 >= 10.0 && lsu0 < 10.00001){
			log_test_(base10, 6.0e-10);
		} else if (lsu0 >= 10.00001 && lsu0 < 10.001){
			log_test_(base10, 6.0e-11);
		} else if (lsu0 >= 10.001 && lsu0 < 10.1){
			log_test_(base10, 1.5e-12);
		} else if (lsu0 >= 10.1 && lsu0 < 11){
			log_test_(base10, 1.6e-14);
		} else if (lsu0 >= 11 && lsu0 < 13){
			log_test_(base10, 2.0e-15);
		} else {
			log_test_(base10, 6.5e-16);
		}
	} else {
		log_test_(base10, 2e-16);
	}
}

static void log_test(
	//input
	const char* x_str, int x_exp,
	bool base10=false
)
{
	CAPTURE(x_str); CAPTURE(x_exp);
	CAPTURE(base10);
	build_dec80(x_str, x_exp);
	log_test(base10);
}

TEST_CASE("log"){
	log_test("0", 0);
	log_test("-1", 0);
	log_test("0.155", 0);
	log_test("10", 0);
	log_test("1.1", 10);
	log_test("2.02", -10);
	log_test("2.02", 0);
	log_test("1.5", 0, true);
	log_test("9", 99);
	log_test("123", 12345);
}

TEST_CASE("log random"){
	std::default_random_engine gen;
	std::uniform_int_distribution<int> distrib(0,99);
	std::uniform_int_distribution<int> exp_distrib(-99,99);
	std::uniform_int_distribution<int> sign_distrib(0,1);
	for (int j = 0; j < NUM_RAND_TESTS; j++){
		for (int i = 0; i < DEC80_NUM_LSU; i++){
			AccDecn.lsu[i] = distrib(gen);
		}
		int exp = exp_distrib(gen);
		set_exponent(&AccDecn, exp, 0);
		int base10 = sign_distrib(gen);
		log_test(base10);
	}
}

static void log_test_near1(int lsu0_low, int lsu0_high, int exp){
	std::default_random_engine gen;
	std::uniform_int_distribution<int> lsu0_distrib(lsu0_low, lsu0_high);
	std::uniform_int_distribution<int> distrib(0,99);
	std::uniform_int_distribution<int> exp_distrib(-99,99);
	std::uniform_int_distribution<int> sign_distrib(0,1);
	for (int j = 0; j < NUM_RAND_TESTS; j++){
		AccDecn.lsu[0] = lsu0_distrib(gen);
		for (int i = 1; i < DEC80_NUM_LSU; i++){
			AccDecn.lsu[i] = distrib(gen);
		}
		set_exponent(&AccDecn, exp, 0);
		int base10 = sign_distrib(gen);
		log_test(base10);
	}
}

TEST_CASE("log random 0 to 0.99..."){
	log_test_near1(0,  99, -1);
}
TEST_CASE("log random 0.8 to 0.99..."){
	log_test_near1(80, 99, -1);
}
TEST_CASE("log random 1.0 to 9.9"){
	log_test_near1(10, 99, 0);
}
TEST_CASE("log random 1.0 to 2.0"){
	log_test_near1(10, 20, 0);
}

static void exp_test(
	//input
	const char* x_str, int x_exp,
	double epsilon=6e-16,
	bool base10=false
)
{
	CAPTURE(x_str); CAPTURE(x_exp);
	CAPTURE(base10);
	build_dec80(x_str, x_exp);
	if (base10){
		exp10_decn();
	} else {
		exp_decn();
	}
	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);  // exp(x)
	CAPTURE(AccDecn.exponent);

	//calculate actual result
	bmp::mpfr_float::default_precision(50);
	bmp::mpfr_float calculated(Buf);
	std::string x_full_str(x_str);
	x_full_str += "e" + std::to_string(x_exp);
	bmp::mpfr_float x_actual(x_full_str);
	if (base10){
		x_actual *= log(10);
	}
	x_actual = exp(x_actual);
	CAPTURE(x_actual);
	bmp::mpfr_float rel_diff = abs((x_actual - calculated) / x_actual);
	CHECK(rel_diff < epsilon);
}

static void exp10_test(
	//input
	const char* x_str, int x_exp,
	double epsilon=3e-15
)
{
	exp_test(x_str, x_exp, epsilon, true);
}

TEST_CASE("exp"){
	exp_test("4.4", 0);
	exp_test("0.155", 0);
	exp_test("9.999", 0);
	exp_test("10", 0);
	exp_test("10.001", 0);
	exp_test("2.3", 2, 6e-15);
	exp_test("2.02", -10);
	exp_test("2.02", 0);
	exp_test("1.5", 0);
	exp_test("294.69999999", 0, 8e-15);

	//do not operate on NaN
	set_dec80_NaN(&AccDecn);
	exp_decn();
	CHECK(decn_is_nan(&AccDecn)); //still NaN
}


TEST_CASE("exp10"){
	exp10_test("4.4", 0);
	exp10_test("0.155", 0);
	exp10_test("9.999", 0);
	exp10_test("10", 0);
	exp10_test("10.001", 0);
	exp10_test("2.02", -10);
	exp10_test("2.02", 0);
	exp10_test("1.5", 0);
	exp10_test("127", 0, 3e-14);
}

static void pow_test(
	//input
	const char* a_str, int a_exp,
	const char* b_str, int b_exp
)
{
	CAPTURE(a_str); CAPTURE(a_exp);
	CAPTURE(b_str); CAPTURE(b_exp);

	//compute power
	build_decn_at(&BDecn,   b_str, b_exp);
	build_dec80(a_str, a_exp);

	pow_decn();

	decn_to_str_complete(&AccDecn);
	CAPTURE(Buf);  // a^b

	//calculate actual result
	bmp::mpfr_float::default_precision(50);
	bmp::mpfr_float calculated(Buf);
	std::string a_full_str(a_str);
	a_full_str += "e" + std::to_string(a_exp);
	std::string b_full_str(b_str);
	b_full_str += "e" + std::to_string(b_exp);;
	// 	CAPTURE(a_full_str);
	// 	CAPTURE(b_full_str);
	bmp::mpfr_float a_actual(a_full_str);
	bmp::mpfr_float b_actual(b_full_str);
	a_actual = pow(a_actual, b_actual);
	if (decn_is_zero(&AccDecn)) {
		bmp::mpfr_float diff = abs(a_actual - calculated);
		CHECK(diff < 3e-14);
	} else {
		bmp::mpfr_float rel_diff = abs((a_actual - calculated)/a_actual);
		CHECK(rel_diff < 3e-14);
	}
}

TEST_CASE("power"){
	pow_test(
		"3.14", 60,
		"-1.5", -2
	);

	pow_test(
		"3", 0,
		"201", 0
	);

	pow_test(
		"5", 0,
		"0", 0
	);

	pow_test(
		"5", 0,
		"0", 2
	);

	pow_test(
		"0", 0,
		"5", 0
	);

	pow_test(
		"0", 0,
		"0", 0
	);
}

TEST_CASE("u32str corner"){
	u32str(0, &Buf[0], 10);
	CHECK_THAT(Buf, Equals("0"));
}
