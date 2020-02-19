// Distributed under the MIT License.
// See LICENSE.txt for details.

#include "tests/Unit/TestingFramework.hpp"

#include <algorithm>
#include <array>
#include <boost/optional/optional_io.hpp>
#include <cstddef>
#include <string>

#include "DataStructures/ComplexDataVector.hpp"
#include "DataStructures/ComplexModalVector.hpp"
#include "DataStructures/DataBox/DataBoxTag.hpp"
#include "DataStructures/DataVector.hpp"
#include "DataStructures/ModalVector.hpp"
#include "DataStructures/Tensor/Slice.hpp"
#include "DataStructures/Tensor/Tensor.hpp"
#include "DataStructures/Variables.hpp"
#include "Utilities/TMPL.hpp"
#include "Utilities/TypeTraits.hpp"
#include "tests/Unit/DataStructures/TestTags.hpp"
#include "tests/Unit/TestHelpers.hpp"
#include "tests/Utilities/MakeWithRandomValues.hpp"

namespace {
template <typename VectorType>
void test_variables_slice() noexcept {
  MAKE_GENERATOR(gen);
  UniformCustomDistribution<size_t> sdist{5, 10};

  const size_t x_extents = sdist(gen);
  const size_t y_extents = sdist(gen);
  const size_t z_extents = sdist(gen);
  Variables<tmpl::list<VariablesTestTags_detail::tensor<VectorType>>> vars{
      x_extents * y_extents * z_extents};
  const size_t tensor_size =
      VariablesTestTags_detail::tensor<VectorType>::type::size();
  Index<3> extents(x_extents, y_extents, z_extents);

  // Test data_on_slice function by using a predictable data set where each
  // entry is assigned a value equal to its index
  for (size_t s = 0; s < vars.size(); ++s) {
    // clang-tidy: do not use pointer arithmetic
    vars.data()[s] = s;  // NOLINT
  }
  Variables<tmpl::list<VariablesTestTags_detail::tensor<VectorType>>>
      expected_vars_sliced_in_x(y_extents * z_extents, 0.),
      expected_vars_sliced_in_y(x_extents * z_extents, 0.),
      expected_vars_sliced_in_z(x_extents * y_extents, 0.);
  const size_t x_offset = sdist(gen) % x_extents;
  const size_t y_offset = sdist(gen) % y_extents;
  const size_t z_offset = sdist(gen) % z_extents;

  for (size_t s = 0; s < expected_vars_sliced_in_x.size(); ++s) {
    // clang-tidy: do not use pointer arithmetic
    expected_vars_sliced_in_x.data()[s] = x_offset + s * x_extents;  // NOLINT
  }
  for (size_t i = 0; i < tensor_size; ++i) {
    for (size_t x = 0; x < x_extents; ++x) {
      for (size_t z = 0; z < z_extents; ++z) {
        // clang-tidy: do not use pointer arithmetic
        expected_vars_sliced_in_y
            .data()[x + x_extents * (z + z_extents * i)] =  // NOLINT
            i * extents.product() + x + x_extents * (y_offset + z * y_extents);
      }
    }
  }
  for (size_t i = 0; i < tensor_size; ++i) {
    for (size_t x = 0; x < x_extents; ++x) {
      for (size_t y = 0; y < y_extents; ++y) {
        // clang-tidy: do not use pointer arithmetic
        expected_vars_sliced_in_z
            .data()[x + x_extents * (y + y_extents * i)] =  // NOLINT
            i * extents.product() + x + x_extents * (y + y_extents * z_offset);
      }
    }
  }

  INFO("Test simple slice");
  CHECK(data_on_slice(get<VariablesTestTags_detail::tensor<VectorType>>(vars),
                      extents, 0, x_offset) ==
        get<VariablesTestTags_detail::tensor<VectorType>>(
            expected_vars_sliced_in_x));
  CHECK(data_on_slice(get<VariablesTestTags_detail::tensor<VectorType>>(vars),
                      extents, 1, y_offset) ==
        get<VariablesTestTags_detail::tensor<VectorType>>(
            expected_vars_sliced_in_y));
  CHECK(data_on_slice(get<VariablesTestTags_detail::tensor<VectorType>>(vars),
                      extents, 2, z_offset) ==
        get<VariablesTestTags_detail::tensor<VectorType>>(
            expected_vars_sliced_in_z));

  INFO("Test slice of a boost::optional<Tensor>");
  REQUIRE(data_on_slice(
      boost::make_optional(
          get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
      extents, 0, x_offset));
  CHECK(data_on_slice(
            boost::make_optional(
                get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
            extents, 0, x_offset)
            .get() == get<VariablesTestTags_detail::tensor<VectorType>>(
                          expected_vars_sliced_in_x));
  REQUIRE(data_on_slice(
      boost::make_optional(
          get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
      extents, 1, y_offset));
  CHECK(data_on_slice(
            boost::make_optional(
                get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
            extents, 1, y_offset)
            .get() == get<VariablesTestTags_detail::tensor<VectorType>>(
                          expected_vars_sliced_in_y));
  REQUIRE(data_on_slice(
      boost::make_optional(
          get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
      extents, 2, z_offset));
  CHECK(data_on_slice(
            boost::make_optional(
                get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
            extents, 2, z_offset)
            .get() == get<VariablesTestTags_detail::tensor<VectorType>>(
                          expected_vars_sliced_in_z));

  CHECK_FALSE(
      data_on_slice(boost::optional<tnsr::I<VectorType, 3, Frame::Inertial>>{},
                    extents, 0, x_offset));

  // Test not_null<boost::option<Tensor>>
  using TensorType =
      db::item_type<VariablesTestTags_detail::tensor<VectorType>>;
  auto optional_tensor = boost::make_optional(TensorType{});
  CHECK(optional_tensor);
  data_on_slice(make_not_null(&optional_tensor), boost::optional<TensorType>{},
                extents, 0, x_offset);
  CHECK_FALSE(optional_tensor);

  data_on_slice(make_not_null(&optional_tensor),
                boost::make_optional(
                    get<VariablesTestTags_detail::tensor<VectorType>>(vars)),
                extents, 0, x_offset);
  REQUIRE(optional_tensor);
  CHECK(optional_tensor.get() ==
        get<VariablesTestTags_detail::tensor<VectorType>>(
            expected_vars_sliced_in_x));

  optional_tensor = boost::none;
  CHECK_FALSE(optional_tensor);
  data_on_slice(make_not_null(&optional_tensor), boost::optional<TensorType>{},
                extents, 0, x_offset);
  CHECK_FALSE(optional_tensor);
}

SPECTRE_TEST_CASE("Unit.DataStructures.Tensor.Slice",
                  "[DataStructures][Unit]") {
  test_variables_slice<ComplexDataVector>();
  test_variables_slice<ComplexModalVector>();
  test_variables_slice<DataVector>();
  test_variables_slice<ModalVector>();
}
}  // namespace
