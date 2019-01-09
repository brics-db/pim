#ifdef __SSE4_2__

#pragma once

#include <zsim_hooks.h>
#include <immintrin.h>

/*
 * AN encoding sub-block - SSE
 * Template specification. We will only provide fully specialized templates.
 * This is required, because we must use type-specific intrinsics.
 */
template<typename T1, typename T2>
void __encodeSSE42(__m128i * mmIn, __m128i * mmEnc, __m128i mmA);

/*
 * AN encoding sub-block - SSE - uint16_t -> uint32_t
 */
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

/*
 * AN encoding - SSE
 * This is the encoding template, which utilizes the above specializations.
 */
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

