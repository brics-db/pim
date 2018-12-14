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

template<typename T1, size_t N1, typename T2, size_t N2, typename T3>
size_t encodeScalar(std::array<T1, N1> & bufIn, std::array<T2, N2> & bufEnc, T3 A) {
	zsim_PIM_function_begin();
	size_t i = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	for (; i < N1; ++i) {
		bufEnc[i] = static_cast<T2>(bufIn[i]) * A;
	}
	zsim_PIM_function_end();
	return i;
}

template<typename T1, size_t N1, typename T2, size_t N2, typename T3, typename B, size_t NB>
size_t decodeCheckedScalar(std::array<T1, N1> & bufEnc, std::array<T2, N2> & bufDec, T3 Ainv, std::array<B, NB> & bitmap) {
	const size_t BITSPERUNIT = sizeof(B) * 8;
	zsim_PIM_function_begin();
	size_t i = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / BITSPERUNIT) <= NB, "Bitmap is too small!");
	for(; i < N1; ++i) {
		T1 val = bufEnc[i] * Ainv;
		bufDec[i] = static_cast<T2>(val);
		if (val > std::numeric_limits<T2>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return i;
}

template<typename T, size_t N1, size_t N2, typename T2, typename B, size_t N3>
size_t filter1LTScalar(std::array<T, N1> & bufEnc, std::array<T, N2> & bufFilt, T2 Ainv, std::array<B, N3> & bitmap, T threshold) {
	const size_t BITSPERUNIT = sizeof(B) * 8;
	zsim_PIM_function_begin();
	size_t i = 0;
	size_t j = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / BITSPERUNIT) <= N3, "Bitmap is too small!");
	for(; i < N1; ++i) {
		T val = static_cast<T>(bufEnc[i] * Ainv);
		if (val < threshold) {
			bufFilt[j++] = val;
		}
		if (val > std::numeric_limits<T>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return j;
}

#ifdef __SSE4_2__
template<typename T1, typename T2>
void __encodeSSE42(__m128i * mmIn, __m128i * mmEnc, __m128i mmA);

template<>
void __encodeSSE42<uint16_t, uint32_t>(__m128i * mmIn, __m128i * mmEnc, __m128i mmA) {
	__m128i mmUnenc = _mm_lddqu_si128(mmIn);
	__m128i mmMid1 = _mm_cvtepu16_epi32(mmUnenc);
	__m128i mmEnc1 = _mm_mullo_epi32(mmMid1, mmA);
	__m128i mmMid2 = _mm_srli_si128(mmUnenc, 8);
	__m128i mmMid3 = _mm_cvtepu16_epi32(mmMid2);
	__m128i mmEnc2 = _mm_mullo_epi32(mmMid3, mmA);
	_mm_storeu_si128(mmEnc, mmEnc1);
	_mm_storeu_si128(mmEnc + 1, mmEnc2);
}

template<typename T1, size_t N1, typename T2, size_t N2, typename T3>
static inline size_t encodeSSE42(std::array<T1, N1> & bufIn, std::array<T2, N2> & bufEnc, T3 A) {
	const size_t ELEMENTS_PER_VECIN = sizeof(__m128i) / sizeof(T1);
	const size_t ELEMENTS_PER_VECENC = sizeof(__m128i) / sizeof(T2);
	zsim_PIM_function_begin();
	size_t i = 0;
	size_t j = 0;
	__m128i mmA = _mm_set1_epi32(A);
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	for (; i <= (N1 - ELEMENTS_PER_VECIN); i += ELEMENTS_PER_VECIN, j += 2 * ELEMENTS_PER_VECENC) {
		/*
		__m128i mmUnenc = _mm_lddqu_si128(reinterpret_cast<__m128i*>(&bufIn[i]));
		__m128i mmEnc1 = _mm_mullo_epi32(_mm_cvtepi16_epi32(mmUnenc), mmA);
		__m128i mmEnc2 = _mm_mullo_epi32(_mm_cvtepi16_epi32(_mm_srli_si128(mmUnenc, 8)), mmA);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(&bufEnc[j]), mmEnc1);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(&bufEnc[j + ELEMENTS_PER_VECENC]), mmEnc2);
		*/
		__encodeSSE42<T1, T2>(reinterpret_cast<__m128i*>(&bufIn[i]), reinterpret_cast<__m128i*>(&bufEnc[j]), mmA);
	}
	for (; i < N1; ++i) {
		bufEnc[i] = static_cast<T2>(bufIn[i]) * A;
	}
	zsim_PIM_function_end();
	return i;
}

template<typename T>
int movemask(__m128i & mmMask);
template<>
int movemask<uint64_t>(__m128i & mmMask) {
	return _mm_movemask_pd(_mm_castsi128_pd(mmMask));
}
template<>
int movemask<uint32_t>(__m128i & mmMask) {
	return _mm_movemask_ps(_mm_castsi128_ps(mmMask));
}
template<>
int movemask<uint8_t>(__m128i & mmMask) {
	return _mm_movemask_epi8(mmMask);
}

template<typename T1, size_t N1, typename T2, size_t N2, typename T3, typename B, size_t NB>
size_t decodeCheckedSSE42(std::array<T1, N1> & bufEnc, std::array<T2, N2> & bufDec, T3 Ainv, std::array<B, NB> & bitmap) {
	const size_t BITSPERUNIT = sizeof(B) * 8;
	const size_t ELEMENTS_PER_VECENC = sizeof(__m128i) / sizeof(T1);
	const size_t RATIO = sizeof(T1) / sizeof(T2);
	const size_t ELEMENTS_PER_VECDEC = (sizeof(__m128i) / sizeof(T2)) / RATIO;
	zsim_PIM_function_begin();
	__m128i mmShuffle = _mm_set_epi64x(0x8080808080808080ULL, 0x0D0C090805040100ULL);
	size_t i = 0;
	size_t j = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / BITSPERUNIT) <= NB, "Bitmap is too small!");
	__m128i mmAinv = _mm_set1_epi32(Ainv);
	__m128i mmMax = _mm_set1_epi32(std::numeric_limits<T2>::max());
	for(; i <= (N1 - ELEMENTS_PER_VECENC); i += ELEMENTS_PER_VECENC, j += ELEMENTS_PER_VECDEC) {
		__m128i mmDec = _mm_mullo_epi32(_mm_lddqu_si128(reinterpret_cast<__m128i*>(&bufEnc[i])), mmAinv);
		*reinterpret_cast<uint64_t*>(&bufDec[j]) =_mm_extract_epi64(_mm_shuffle_epi8(mmDec, mmShuffle), 0);
		__m128i mmMask = _mm_cmpgt_epi32(mmDec, mmMax);
		int mask = movemask<T1>(mmMask);
		if (mask) {
			std::cerr << i << ':' << mask << std::endl;
			for(size_t k = 0; k < sizeof(T1); ++k) {
				if (mask & (0x1 << k)) {
					bitmap[(i + k) / BITSPERUNIT] |= 0x1 << (i + k) % BITSPERUNIT;
				}
			}
		}
	}
	for(; i < N1; ++i) {
		bufDec[i] = bufEnc[i] * Ainv;
		if (bufDec[i] > std::numeric_limits<T2>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return i;
}

template<typename T, size_t N1, size_t N2, typename T2, typename B, size_t N3>
size_t filter1LTSSE42(std::array<T, N1> & bufEnc, std::array<T, N2> & bufFilt, T2 Ainv, std::array<B, N3> & bitmap, T threshold) {
	const size_t BITSPERUNIT = sizeof(B) * 8;
	const size_t ELEMENTS_PER_VECENC = sizeof(__m128i) / sizeof(T);
	zsim_PIM_function_begin();
	size_t i = 0;
	size_t j = 0;
	static_assert(N1 <= N2, "Second array must be of greater or equal size as the first array!");
	static_assert((N1 / BITSPERUNIT) <= N3, "Bitmap is too small!");
	__m128i mmAinv = _mm_set1_epi32(Ainv);
	__m128i mmMax = _mm_set1_epi32(std::numeric_limits<uint_t>::max());
	__m128i mmThreshold = _mm_set1_epi32(threshold);
	for(; i <= (N1 - ELEMENTS_PER_VECENC); i += ELEMENTS_PER_VECENC) {
		__m128i mmEnc = _mm_lddqu_si128(reinterpret_cast<__m128i*>(&bufEnc[i]));
		__m128i mmDec = _mm_mullo_epi32(mmEnc, mmAinv);
		__m128i mmMask = _mm_cmplt_epi32(mmEnc, mmThreshold);
		int mask = movemask<T>(mmMask);
		if (mask) {
			for(size_t k = 0; k < sizeof(T); ++k) {
				if (mask & (0x1 << k)) {
					bufFilt[j++] = _mm_extract_epi32(mmEnc, k);
				}
			}
		}
		mmMask = _mm_cmpgt_epi32(mmDec, mmMax);
		mask = movemask<T>(mmMask);
		if (mask) {
			std::cerr << i << ':' << mask << std::endl;
			for(size_t k = 0; k < sizeof(uint_t); ++k) {
				if (mask & (0x1 << k)) {
					bitmap[(i + k) / BITSPERUNIT] |= 0x1 << (i + k) % BITSPERUNIT;
				}
			}
		}
	}
	for(; i < N1; ++i) {
		T val = static_cast<T>(bufEnc[i] * Ainv);
		if (val < threshold) {
			bufFilt[j++] = val;
		}
		if (val > std::numeric_limits<uint_t>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_PIM_function_end();
	return j;
}
#endif

int main(int argc, char *argv[]) {
	const size_t SIZE = 1'000'000;

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

#ifdef __SSE4_2__
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
