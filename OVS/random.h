/*
* Copyright (c) 2008, 2009, 2010, 2012 Nicira, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef RANDOM_H
#define RANDOM_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <random>
#include "ElementaryClasses.h"
using namespace std;

std::mt19937 generator;

	int random_int(int low, int high)
	{
		//static std::mt19937  generator;
		using Dist = std::uniform_int_distribution < int >;
		Dist uid{};
		return uid(generator, Dist::param_type{ low, high });
	}

	// random number generator from Stroustrup:
	// http://www.stroustrup.com/C++11FAQ.html#std-random
	// static: there is only one initialization (and therefore seed).
	int random_unsigned_int()
	{
		//static std::mt19937  generator;
		using Dist = std::uniform_int_distribution < unsigned int >;
		Dist uid{};
		return uid(generator, Dist::param_type{ 0, 4294967295 });
	}
	double random_real_btw_0_1()
	{
		//static std::mt19937  generator;
		using Dist = std::uniform_real_distribution < double >;
		Dist uid{};
		return uid(generator, Dist::param_type{ 0,1 });
	}

	template <class T>
	std::vector<T> shuffle_vector(std::vector<T> vi) {
		//static std::mt19937  generator;
		std::shuffle(std::begin(vi), std::end(vi), generator);
		return vi;
	}



void random_init(void);
void random_set_seed(uint32_t);

void random_bytes(void *, size_t);
uint32_t random_uint32(void) {
	return random_unsigned_int();
}
uint64_t random_uint64(void) {
	printf("warning random_uint64 unimplemented\n");
	return random_unsigned_int();
}

static inline int
random_range(int max)
{
	return random_unsigned_int() % max;
}

static inline uint8_t
random_uint8(void)
{
	return random_unsigned_int();
}

static inline uint16_t
random_uint16(void)
{
	return random_unsigned_int();
}

#endif /* random.h */