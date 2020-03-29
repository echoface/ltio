
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
