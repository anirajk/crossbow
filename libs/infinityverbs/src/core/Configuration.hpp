/*
 * Configuration.hpp
 *
 *  Created on: Nov 25, 2014
 *      Author: claudeb
 */

#ifndef INFINITYVERBS_SRC_CORE_CONFIGURATION_HPP_
#define INFINITYVERBS_SRC_CORE_CONFIGURATION_HPP_

#include <stdlib.h>
#include <stdio.h>
#include <cassert>

#define INFINITY_VERBS_DEFAULT_SEND_QUEUE_LENGTH (32)

#define INFINITY_VERBS_DEFAULT_RECEIVE_QUEUE_LENGTH (32)

#define INFINITY_VERBS_DEFAULT_TOKEN_STORE_SIZE (1024)

#define INFINITY_VERBS_DEFAULT_DEVICE_NAME "ib0"

#define INFINITY_VERBS_DEFAULT_DEVICE (0)

#define INFINITY_VERBS_DEFAULT_DEVICE_PORT (1)

#define INFINITY_VERBS_DEBUG_LEVEL (2)

#define INFINITY_VERBS_MEMORY_TOKEN_STORE_SIZE (64)

#define INFINITY_VERBS_PAGE_ALIGNMENT (64*1024)

#define INFINITY_VERBS_BUFFER_ALIGNMENT (64)

#define INFINITY_VERBS_DEBUG_STATUS(L, X, ...) {if(INFINITY_VERBS_DEBUG_LEVEL>L) {fprintf(stdout, X, ##__VA_ARGS__); fflush(stdout);}}

#define INFINITY_VERBS_ASSERT(B, X, ...) {if(!(B)) {fprintf(stdout, X, ##__VA_ARGS__); fflush(stdout); exit(-1);}}

#endif /* INFINITYVERBS_SRC_CORE_CONFIGURATION_HPP_ */