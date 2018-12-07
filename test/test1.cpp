#include <zsim_hooks.h>
#include <iostream>
#include <cstdint>
#include <string>
#include <array>
#include <exception>
#include <random>
#include <limits>

typedef uint16_t A_t;
typedef uint64_t resint_t;
static const constexpr std::array<A_t, 16> As = {1, 3, 7, 13, 29, 61, 125, 225, 445, 881, 2029, 3973, 7841, 16089, 32417, 65117};
static const constexpr std::array<resint_t, 16> AInvs = {1, 0xAAAAAAAAAAAAAAAB, 0x6DB6DB6DB6DB6DB7, 0x4EC4EC4EC4EC4EC5, 0x34F72C234F72C235, 0x4FBCDA3AC10C9715, 0x1CAC083126E978D5,
	0x0FEDCBA987654321, 0x64194FF6CBA64195, 0xC87FDACE4F9E5D91, 0x49AEFF9F19DD6DE5, 0xA6B1742D1ABD914D, 0x3434A14919D34561, 0x227F016A87066169, 0x73B9563A744AE561, 0x3E36D3BC8D9BC5F5};

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

	const size_t SIZE = 1024 * 1024;
	uint64_t * bufferIn = new uint64_t[SIZE]{};
	uint64_t * bufferEnc = new uint64_t[SIZE]{};
	uint64_t * bufferOut = new uint64_t[SIZE]{};
	const size_t BITSPERUNIT = sizeof(uint64_t) * 8;
	uint64_t * bitmap = new uint64_t[SIZE / BITSPERUNIT]{};
	for(size_t i = 0; i < SIZE; ++i) {
		bufferIn[i] = distribution(generator); // slow, but random ;-) -- yes we can use an xor-shuffle-generator instead...
	}
	for(size_t i = 0; i < (SIZE / BITSPERUNIT); ++i) {
		bitmap[i] = 0;
	}

	std::cout << "start." << std::endl;
	zsim_roi_begin();
	zsim_PIM_function_begin();
	zsim_work_begin();
	COMPILER_BARRIER()
	for (size_t i = 0; i < SIZE; ++i)
		bufferEnc[i] = bufferIn[i] * A;
	COMPILER_BARRIER()
	for(size_t i = 0; i < SIZE; ++i) {
		bufferOut[i] = bufferEnc[i] * Ainv;
		COMPILER_BARRIER()
		if (bufferOut[i] > std::numeric_limits<uint32_t>::max()) {
			bitmap[i / BITSPERUNIT] |= 0x1 << (i % BITSPERUNIT);
		}
	}
	zsim_work_end();
	zsim_PIM_function_end();
	zsim_roi_end();

	for(size_t i = 0; i < SIZE; ++i) {
		if (bufferIn[i] != bufferOut[i]) {
			std::cout << i << ": " << bufferIn[i] << " != " << bufferOut[i] << '\n';
		}
	}
	for (size_t i = 0; i < (SIZE / BITSPERUNIT); ++i) {
		if (bitmap[i]) {
			std::cout << "error found at " << (i * BITSPERUNIT) << ": " << bitmap[i] << '\n';
		}
	}

	std::cout << "end." << std::endl;
}
