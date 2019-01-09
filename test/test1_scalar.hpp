#include <zsim_hooks.h>

#pragma once

/*
 * AN encoding - scalar
 */
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

/*
 * AN decoding with error checking - scalar
 */
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

/*
 * Filtering An encoded data -- including error checks - scalar
 */
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

