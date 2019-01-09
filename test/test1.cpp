#include <zsim_hooks.h>
#include <iostream>
#include <cstdint>
#include <string>
#include <array>
#include <exception>
#include <random>
#include <limits>
#include <immintrin.h>
#include <vector>

typedef uint16_t A_t;
#if false
typedef uint32_t uint_t;
typedef uint64_t resuint_t;
static const constexpr std::array<A_t, 16> As = {1, 3, 7, 13, 29, 61, 125, 225, 445, 881, 2029, 3973, 7841, 16089, 32417, 65117};
static const constexpr std::array<resuint_t, 16> AInvs = {1, 0xAAAAAAAAAAAAAAAB, 0x6DB6DB6DB6DB6DB7, 0x4EC4EC4EC4EC4EC5, 0x34F72C234F72C235, 0x4FBCDA3AC10C9715, 0x1CAC083126E978D5,
	0x0FEDCBA987654321, 0x64194FF6CBA64195, 0xC87FDACE4F9E5D91, 0x49AEFF9F19DD6DE5, 0xA6B1742D1ABD914D, 0x3434A14919D34561, 0x227F016A87066169, 0x73B9563A744AE561, 0x3E36D3BC8D9BC5F5};
#else
typedef uint16_t uint_t;
typedef uint32_t resuint_t;
const constexpr std::array<A_t, 16> As = {1, 3, 7, 13, 29, 61, 119, 233, 463, 947, 1939, 3349, 7785, 14781, 28183, 63877};
const constexpr std::array<resuint_t, 16> AInvs = {0x00000001, 0xaaaaaaab, 0xb6db6db7, 0xc4ec4ec5, 0x4f72c235, 0xc10c9715, 0x46fdd947, 0x1fdcd759, 0xab67652f, 0xff30637b, 0xbc452e9b, 0x21b5da3d,
        0x392f51d9, 0x1abdc995, 0xab2da9a7, 0xd142174d};
#endif



/*
 * Yes, this is clumsy to include here, but we need the above definition of "uint_t" for the SSE code.
 * To avoid this, we would need to use a type system as in BRICS-DB (aka AHEAD).
 */
#include "test1_scalar.hpp"
#include "test1_SSE.hpp"



/*
 * Main function -- all real functionality is in test1_scalar.hpp and test1_SSE.hpp
 */
int main(int argc, char *argv[]) {
	const size_t SIZE = 10'000;

	int idx = 1;
	A_t A = As[idx];
	if (argc > 1) {
		idx = std::stoi(std::string(argv[1]));
		if (idx < 0)
			throw std::runtime_error("The A-index must be larger than 0.");
		if (size_t(idx) > (As.size() - 1))
			throw std::runtime_error("The A-index is too large.");
		A = As[idx];
	}
	resuint_t Ainv = AInvs[idx];

	std::cout << "sizeof(uint_t)=" << sizeof(uint_t) << ". sizeof(resuint_t)=" << sizeof(resuint_t) << std::endl;
	std::cout << "Using A=" << A << " and A^-1=" << Ainv << ". A*A^-1=" << (resuint_t)(A * Ainv) << std::endl;
	std::cout << "SIZE=" << SIZE << std::endl;

	//std::default_random_engine generator;
	std::mt19937 generator(1); // initial seed of "1", for the sake of reproducibility
	std::uniform_int_distribution<uint_t> distribution;

	auto bufferIn = new std::array<uint_t, SIZE>{};
	auto bufferEnc = new std::array<resuint_t, SIZE>{};
	auto bufferFilt = new std::array<resuint_t, SIZE>{};
	auto bufferDec = new std::array<uint_t, SIZE>{};
	const size_t BITSPERUNIT = sizeof(size_t) * 8;
	auto bitmapDec = new std::array<size_t, SIZE / BITSPERUNIT>{};
	auto bitmapFilt = new std::array<size_t, SIZE / BITSPERUNIT>{};
	std::cout << "filling array." << std::endl;
	for(size_t i = 0; i < SIZE; ++i) {
		bufferIn[0][i] = distribution(generator); // slow, but random ;-) -- yes we can use an xor-shuffle-generator instead...
	}

#if defined(TEST_SSE) || defined(__SSE4_2__) && ! defined(TEST_SCALAR)
#define encode encodeSSE42
#define decodeChecked decodeCheckedSSE42
#define filter1LT filter1LTScalar
	std::cout << "SSE4.2" << std::endl;
#else
#define encode encodeScalar
#define decodeChecked decodeCheckedScalar
#define filter1LT filter1LTSSE42
	std::cout << "Scalar" << std::endl;
#endif
	std::cout << "start." << std::endl;
	zsim_roi_begin();
	size_t num = encode(*bufferIn, *bufferEnc, A);
	if (num != bufferIn->size()) {
		std::cerr << "[WARNING] After encode(): " << num << " != " << bufferIn->size() << std::endl;
	}
	num = decodeChecked(*bufferEnc, *bufferDec, Ainv, *bitmapDec);
	if (num != bufferEnc->size()) {
		std::cerr << "[WARNING] After decodeChecked(): " << num << " != " << bufferEnc->size() << std::endl;
	}
	size_t numFilt = filter1LT(*bufferEnc, *bufferFilt, Ainv, *bitmapFilt, static_cast<resuint_t>(0x1234 * A));
	zsim_roi_end();
	std::cout << "Filter less-than: " << numFilt << " elements out of " << SIZE << " matched (" << ((static_cast<double>(numFilt) / SIZE) * 100) << "%)." << std::endl;
	std::cout << "end.\nchecking." << std::endl;

#undef encode
#undef decodeChecked
#undef filter1LT

	for(size_t i = 0; i < SIZE; ++i) {
		resuint_t expected = static_cast<resuint_t>(bufferIn[0][i]) * static_cast<resuint_t>(A);
		if (static_cast<resuint_t>(bufferEnc[0][i]) != expected) {
			std::cerr << "Error in encode at " << i << ": " << std::hex << std::showbase << bufferEnc[0][i] << " != " << expected << " (" << bufferIn[0][i] << '*' << A << std::dec << std::noshowbase << ')' << '\n';
		}
	}
	for(size_t i = 0; i < SIZE; ++i) {
		if (static_cast<resuint_t>(bufferIn[0][i]) != bufferDec[0][i]) {
			std::cerr << "Error in decode at " << i << ": " << bufferIn[0][i] << " != " << bufferDec[0][i] << '\n';
		}
	}
	for (size_t i = 0; i < (SIZE / BITSPERUNIT); ++i) {
		if (bitmapDec[0][i]) {
			std::cerr << "Error found in decodeChecked at " << (i * BITSPERUNIT) << ": " << std::hex << std::showbase << bitmapDec[0][i] << std::dec << std::noshowbase << '\n';
		}
	}
	for (size_t i = 0; i < (numFilt / BITSPERUNIT); ++i) {
		if (bitmapFilt[0][i]) {
			std::cerr << "Error found in filter1LT at " << (i * BITSPERUNIT) << ": " << std::hex << std::showbase << bitmapFilt[0][i] << std::dec << std::noshowbase << '\n';
		}
	}
}
