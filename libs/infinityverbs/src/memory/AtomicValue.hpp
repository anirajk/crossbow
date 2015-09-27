/*
 * (C) Copyright 2015 ETH Zurich Systems Group (http://www.systems.ethz.ch/) and others.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 *     Markus Pilman <mpilman@inf.ethz.ch>
 *     Simon Loesing <sloesing@inf.ethz.ch>
 *     Thomas Etter <etterth@gmail.com>
 *     Kevin Bocksrocker <kevin.bocksrocker@gmail.com>
 *     Lucas Braun <braunl@inf.ethz.ch>
 */
/*
 * AtomicValue.hpp
 *
 *  Created on: Feb 17, 2015
 *      Author: claudeb
 */

#ifndef MEMORY_ATOMICVALUE_HPP_
#define MEMORY_ATOMICVALUE_HPP_

#include "../core/Configuration.hpp"
#include "MemoryRegion.hpp"

namespace infinityverbs {
namespace memory {

class AtomicValue : public MemoryRegion {

	public:

		AtomicValue(infinityverbs::core::Context *context);
		~AtomicValue();

		uint64_t readValue();

		uint64_t getAddress();
		MemoryToken * getMemoryToken(uint32_t userToken);

	protected:

		uint64_t value;

};

} /* namespace memory */
} /* namespace infinityverbs */

#endif /* MEMORY_ATOMICVALUE_HPP_ */
