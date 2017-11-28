/**
 * @brief Yield structure
 * @file yield.h
 * @author Ran Bao
 * @date 17, Oct, 2017
 */

#ifndef YIELD_H_
#define YIELD_H_

/**
 * @brief The data flow enclosed by YIELD can be stopped by calling break
 */
#define YIELD(expression) \
        do { expression } while (0)

#endif // YIELD_H_
