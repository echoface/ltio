/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef UTILS_ARRAYUTILS_H
#define UTILS_ARRAYUTILS_H

#include <cstddef>

namespace base {

class ArrayUtils {
public:
	/**
	 * Size of a static array
	 *
	 * @param a The array
	 * @return The size (number of elements) of the array
	 */
	template<typename T, size_t N>
	static size_t size(const T (&a)[N]) {
		return N;
	}

	/**
	 * Size of a zero length static array
	 *
	 * @param a The array
	 * @return 0
	 */
	template<typename T>
	static size_t size(const T (&a)[0]) {
		return 0;
	}

	/**
	 * Size of a container
	 *
	 * @param a A container of any type
	 *
	 * @note This function has the same signature as the function for
	 *  static arrays and can be used with automatic template argument deduction.
	 */
	template<typename T>
	static size_t size(const T &a) {
		return a.size();
	}
};

}
#endif // UTILS_ARRAYUTILS_H
