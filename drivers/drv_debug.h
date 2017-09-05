/**
 * @brief Debug macro for drivers
 */

#ifndef DRV_DEBUG_H_
#define DRV_DEBUG_H_

#include BOARD_HEADER

// assert
#ifndef DRV_ASSERT
#define DRV_ASSERT(x) {} while (1)
#endif

#endif /* DRV_DEBUG_H_ */
