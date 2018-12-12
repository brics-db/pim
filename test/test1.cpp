#include <zsim_hooks.h>
#include <iostream>
#include <cstdint>
#include <string>
#include <array>
#include <exception>
#include <random>
#include <limits>
#include <immintrin.h>

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

typedef resuint_t buf_t;

template<typename T, size_t N1, size_t N2>
size_t encodeScalar(std::array<T, N1> & bufIn, std::array<T, N2> & bufEnc, resuint_t A) {
	zsim_PIM_function_begin();
	size_t i = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	for (; i < N1; ++i) {
		bufEnc[i] = bufIn[i] * A;
	}
	zsim_PIM_function_end();
	return i;
}

template<size_t BITSPERUNIT, typename T, size_t N1, size_t N2, size_t N3>
size_t decodeCheckedScalar(std::array<T, N1> & bufEnc, std::array<T, N2> & bufOut, resuint_t Ainv, std::array<size_t, N3> & bitmap) {
	zsim_PIM_function_begin();
	size_t i = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / (sizeof(size_t) * 8)) <= N3, "Bitmap is too small!");
	for(; i < N1; ++i) {
		bufOut[i] = bufEnc[i] * Ainv;
		if (bufOut[i] > std::numeric_limits<uint_t>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return i;
}

#ifdef __SSE4_2__
template<typename T, size_t N1, size_t N2>
static inline size_t encodeSSE42(std::array<T, N1> & bufIn, std::array<T, N2> & bufEnc, resuint_t A) {
	zsim_PIM_function_begin();
	const size_t ELEMENTS_PER_VECTOR = sizeof(__m128i) / sizeof(T);
	size_t i = 0;
	__m128i mmA = _mm_set1_epi32(A);
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	for (; i < N1; i += ELEMENTS_PER_VECTOR) {
		__m128i mmEnc = _mm_mullo_epi32(_mm_lddqu_si128(reinterpret_cast<__m128i*>(&bufIn[i])), mmA);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(&bufEnc[i]), mmEnc);
	}
	zsim_PIM_function_end();
	return i;
}

template<size_t BITSPERUNIT, typename T, size_t N1, size_t N2, size_t N3>
size_t decodeCheckedSSE42(std::array<T, N1> & bufEnc, std::array<T, N2> & bufOut, resuint_t Ainv, std::array<size_t, N3> & bitmap) {
	zsim_PIM_function_begin();
	size_t i = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / (sizeof(size_t) * 8)) <= N3, "Bitmap is too small!");
	__m128i mmAinv = _mm_set1_epi32(Ainv);
	__m128i mmMax = _mm_set1_epi32(std::numeric_limits<uint_t>::max());
	for(; i <= (N1 - sizeof(uint_t)); ++i) {
		__m128i mmDec = _mm_mullo_epi32(_mm_lddqu_si128(reinterpret_cast<__m128i*>(&bufEnc[i])), mmAinv);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(&bufOut[i]), mmDec);
		__m128i mmMask = _mm_cmpgt_epi32(mmDec, mmMax);
		int mask;
		if constexpr (sizeof(uint_t) == 8) {
			mask = _mm_movemask_pd(_mm_castsi128_pd(mmMask));
		} else if constexpr (sizeof(uint_t) == 4) {
			mask = _mm_movemask_ps(_mm_castsi128_ps(mmMask));
		} else if constexpr (sizeof(uint_t) < 4) {
			mask = _mm_movemask_epi8(mmMask);
		}
		if (mask) {
			for(size_t j = 0; i < sizeof(uint_t); ++i) {
				if (mask & (0x1 << j)) {
					bitmap[(i + j) / BITSPERUNIT] |= 0x1 << (i + j) % BITSPERUNIT;
				}
			}
		}
	}
	for(; i < N1; ++i) {
		bufOut[i] = bufEnc[i] * Ainv;
		if (bufOut[i] > std::numeric_limits<uint_t>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return i;
}
#endif

int main(int argc, char *argv[]) {
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
	uint64_t Ainv = AInvs[idx];

	std::default_random_engine generator;
	std::uniform_int_distribution<uint32_t> distribution;

	const size_t SIZE = 1'000'000;
	auto bufferIn = new std::array<buf_t, SIZE>{};
	auto bufferEnc = new std::array<buf_t, SIZE>{};
	auto bufferOut = new std::array<buf_t, SIZE>{};
	const size_t BITSPERUNIT = sizeof(size_t) * 8;
	auto bitmap = new std::array<size_t, SIZE / BITSPERUNIT>{};
	std::cout << "filling array." << std::endl;
	for(size_t i = 0; i < SIZE; ++i) {
		bufferIn[0][i] = distribution(generator); // slow, but random ;-) -- yes we can use an xor-shuffle-generator instead...
	}
	for(size_t i = 0; i < (SIZE / BITSPERUNIT); ++i) {
		bitmap[0][i] = 0;
	}
#ifdef __SSE4_2__
#define encode encodeSSE42
#define decodeChecked decodeCheckedSSE42
#else
#define encode encodeScalar
#define decodeChecked decodeCheckedScalar
#endif
	std::cout << "start." << std::endl;
	zsim_roi_begin();
	size_t num = encode(*bufferIn, *bufferEnc, A);
	if (num != bufferIn->size()) {
		std::cerr << "[WARNING] After encode(): " << num << " != " << bufferIn->size() << std::endl;
	}
	num = decodeChecked<BITSPERUNIT>(*bufferEnc, *bufferOut, Ainv, *bitmap);
	if (num != bufferEnc->size()) {
		std::cerr << "[WARNING] After decodeChecked(): " << num << " != " << bufferEnc->size() << std::endl;
	}
	for(size_t i = 0; i < SIZE; ++i) {
		if (bufferIn[0][i] != bufferOut[0][i]) {
			std::cout << i << ": " << bufferIn[0][i] << " != " << bufferOut[0][i] << '\n';
		}
	}
	for (size_t i = 0; i < (SIZE / BITSPERUNIT); ++i) {
		if (bitmap[0][i]) {
			std::cout << "error found at " << (i * BITSPERUNIT) << ": " << bitmap[0][i] << '\n';
		}
	}
	zsim_roi_end();
	std::cout << "end." << std::endl;
#undef encode
#undef decodeChecked
}
