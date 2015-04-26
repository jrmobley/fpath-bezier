
#include "fpath_builder.h"

// Angle below which we're not going to process with recursion
int32_t max_angle_tolerance = (TRIG_MAX_ANGLE / 360) * 10;

bool recursive_bezier_fixed(FPathBuilder *builder,
                            fixed_t x1, fixed_t y1,
                            fixed_t x2, fixed_t y2,
                            fixed_t x3, fixed_t y3,
                            fixed_t x4, fixed_t y4) {
  // Calculate all the mid-points of the line segments
  fixed_t x12   = (x1 + x2) / 2;
  fixed_t y12   = (y1 + y2) / 2;
  fixed_t x23   = (x2 + x3) / 2;
  fixed_t y23   = (y2 + y3) / 2;
  fixed_t x34   = (x3 + x4) / 2;
  fixed_t y34   = (y3 + y4) / 2;
  fixed_t x123  = (x12 + x23) / 2;
  fixed_t y123  = (y12 + y23) / 2;
  fixed_t x234  = (x23 + x34) / 2;
  fixed_t y234  = (y23 + y34) / 2;
  fixed_t x1234 = (x123 + x234) / 2;
  fixed_t y1234 = (y123 + y234) / 2;

  // Angle Condition
  int32_t a23 = atan2_lookup((int16_t)((y3 - y2) / FIXED_POINT_SCALE),
                             (int16_t)((x3 - x2) / FIXED_POINT_SCALE));
  int32_t da1 = abs(a23 - atan2_lookup((int16_t)((y2 - y1) / FIXED_POINT_SCALE),
                                       (int16_t)((x2 - x1) / FIXED_POINT_SCALE)));
  int32_t da2 = abs(atan2_lookup((int16_t)((y4 - y3) / FIXED_POINT_SCALE),
                                 (int16_t)((x4 - x3) / FIXED_POINT_SCALE)) - a23);

  if (da1 >= TRIG_MAX_ANGLE) {
    da1 = TRIG_MAX_ANGLE - da1;
  }

  if (da2 >= TRIG_MAX_ANGLE) {
    da2 = TRIG_MAX_ANGLE - da2;
  }

  if (da1 + da2 < max_angle_tolerance) {
    // Finally we can stop the recursion
    return fpath_builder_line_to_point(builder, FPoint(x1234, y1234));
  }

  // Continue subdivision if points are being added successfully
  if (recursive_bezier_fixed(builder, x1, y1, x12, y12, x123, y123, x1234, y1234)
      && recursive_bezier_fixed(builder, x1234, y1234, x234, y234, x34, y34, x4, y4)) {
    return true;
  }

  return false;
}

bool bezier_fixed(FPathBuilder* builder, FPoint p1, FPoint p2, FPoint p3, FPoint p4) {
  if (recursive_bezier_fixed(builder, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y)) {
    return fpath_builder_line_to_point(builder, p4);
  }
  return false;
}

FPathBuilder* fpath_builder_create(uint32_t max_points) {
  // Allocate enough memory to store all the points - points are stored contiguously with the
  // FPathBuilder structure
  const size_t required_size = sizeof(FPathBuilder) + max_points * sizeof(FPoint);
  FPathBuilder* result = malloc(required_size);

  if (!result) {
    return NULL;
  }

  memset(result, 0, required_size);
  result->max_points = max_points;
  return result;
}

void fpath_builder_destroy(FPathBuilder* builder) {
  free(builder);
}

FPath* fpath_builder_create_path(FPathBuilder* builder) {
  if (builder->num_points <= 1) {
    return NULL;
  }

  uint32_t num_points = builder->num_points;

  // handle case where last point == first point => remove last point
  while (num_points > 1 && fpoint_equal(&builder->points[0], &builder->points[num_points])) {
    num_points--;
  }

  // Allocate enough memory for both the FPath structure as well as the array of FPoints.
  // Both will be contiguous in memory.
  const size_t size_of_points = num_points * sizeof(FPoint);
  FPath *result = malloc(sizeof(FPath) + size_of_points);

  if (!result) {
    return NULL;
  }

  memset(result, 0, sizeof(FPath));
  result->num_points = num_points;
  // Set the points pointer within the FPath structure to point just after the FPath structure
  // since that is where memory has been allocated for the array.
  result->points = (FPoint*)(result + 1);
  memcpy(result->points, builder->points, size_of_points);
  return result;
}

GPath* fpath_builder_create_gpath(FPathBuilder* builder) {
  if (builder->num_points <= 1) {
    return NULL;
  }

  uint32_t num_points = builder->num_points;

  // handle case where last point == first point => remove last point
  while (num_points > 1 && fpoint_equal(&builder->points[0], &builder->points[num_points])) {
    num_points--;
  }

  // Allocate enough memory for both the FPath structure as well as the array of FPoints.
  // Both will be contiguous in memory.
  const size_t size_of_points = num_points * sizeof(GPoint);
  GPath *result = malloc(sizeof(FPath) + size_of_points);

  if (!result) {
    return NULL;
  }

  memset(result, 0, sizeof(FPath));
  result->num_points = num_points;
  // Set the points pointer within the GPath structure to point just after the GPath structure
  // since that is where memory has been allocated for the array.
  result->points = (GPoint*)(result + 1);
  for (uint16_t k = 0; k < num_points; ++k) {
    result->points[k].x = FIXED_TO_INT(builder->points[k].x);
    result->points[k].y = FIXED_TO_INT(builder->points[k].y);
  }
  return result;
}

bool fpath_builder_move_to_point(FPathBuilder* builder, FPoint to_point) {
  if (builder->num_points != 0) {
    return false;
  }

  return fpath_builder_line_to_point(builder, to_point);
}

bool fpath_builder_line_to_point(FPathBuilder* builder, FPoint to_point) {
  if (builder->num_points >= builder->max_points - 1) {
    return false;
  }

  builder->points[builder->num_points++] = to_point;
  return true;
}

bool fpath_builder_curve_to_point(FPathBuilder* builder, FPoint to_point,
                                  FPoint control_point_1, FPoint control_point_2) {
  FPoint from_point = builder->points[builder->num_points-1];
  return bezier_fixed(builder, from_point, control_point_1, control_point_2, to_point);
}
