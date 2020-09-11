#define G_LOG_DOMAIN "phoc-utils"

#include <wlr/version.h>
#include "utils.h"

void phoc_utils_fix_transform(enum wl_output_transform *transform)
{
  /*
   * Starting from version 0.11.0, wlroots rotates counter-clockwise, while
   * it was rotating clockwise previously.
   * In order to maintain the same behavior, we need to modify the transform
   * before applying it
   */
#if (WLR_VERSION_MAJOR > 0 || WLR_VERSION_MINOR > 10)
  switch(*transform) {
    case WL_OUTPUT_TRANSFORM_90:
      *transform = WL_OUTPUT_TRANSFORM_270;
      break;
    case WL_OUTPUT_TRANSFORM_270:
      *transform = WL_OUTPUT_TRANSFORM_90;
      break;
    case WL_OUTPUT_TRANSFORM_FLIPPED_90:
      *transform = WL_OUTPUT_TRANSFORM_FLIPPED_270;
      break;
    case WL_OUTPUT_TRANSFORM_FLIPPED_270:
      *transform = WL_OUTPUT_TRANSFORM_FLIPPED_90;
      break;
    default:
      /* Nothing to be done */
      break;
  }
#endif
}
