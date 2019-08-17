// Copyright 2019 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Eigen/Core"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/matrix.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {

// Subtract input matrix from the side input matrix and vice versa. The matrices
// must have the same dimension.
// Based on the tag (MINUEND vs SUBTRAHEND), the matrices in the input stream
// and input side packet can be either subtrahend or minuend. The output matrix
// is generated by performing minuend matrix - subtrahend matrix.
//
// Example config:
// node {
//   calculator: "MatrixSubtractCalculator"
//   input_stream: "MINUEND:input_matrix"
//   input_side_packet: "SUBTRAHEND:side_matrix"
//   output_stream: "output_matrix"
// }
//
// or
//
// node {
//   calculator: "MatrixSubtractCalculator"
//   input_stream: "SUBTRAHEND:input_matrix"
//   input_side_packet: "MINUEND:side_matrix"
//   output_stream: "output_matrix"
// }
class MatrixSubtractCalculator : public CalculatorBase {
 public:
  MatrixSubtractCalculator() {}
  ~MatrixSubtractCalculator() override {}

  static ::mediapipe::Status GetContract(CalculatorContract* cc);

  ::mediapipe::Status Open(CalculatorContext* cc) override;
  ::mediapipe::Status Process(CalculatorContext* cc) override;

 private:
  bool subtract_from_input_ = false;
};
REGISTER_CALCULATOR(MatrixSubtractCalculator);

// static
::mediapipe::Status MatrixSubtractCalculator::GetContract(
    CalculatorContract* cc) {
  if (cc->Inputs().NumEntries() != 1 ||
      cc->InputSidePackets().NumEntries() != 1) {
    return ::mediapipe::InvalidArgumentError(
        "MatrixSubtractCalculator only accepts exactly one input stream and "
        "one "
        "input side packet");
  }
  if (cc->Inputs().HasTag("MINUEND") &&
      cc->InputSidePackets().HasTag("SUBTRAHEND")) {
    cc->Inputs().Tag("MINUEND").Set<Matrix>();
    cc->InputSidePackets().Tag("SUBTRAHEND").Set<Matrix>();
  } else if (cc->Inputs().HasTag("SUBTRAHEND") &&
             cc->InputSidePackets().HasTag("MINUEND")) {
    cc->Inputs().Tag("SUBTRAHEND").Set<Matrix>();
    cc->InputSidePackets().Tag("MINUEND").Set<Matrix>();
  } else {
    return ::mediapipe::InvalidArgumentError(
        "Must specify exactly one minuend and one subtrahend.");
  }
  cc->Outputs().Index(0).Set<Matrix>();
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatrixSubtractCalculator::Open(CalculatorContext* cc) {
  // The output is at the same timestamp as the input.
  cc->SetOffset(TimestampDiff(0));
  if (cc->Inputs().HasTag("MINUEND")) {
    subtract_from_input_ = true;
  }
  return ::mediapipe::OkStatus();
}

::mediapipe::Status MatrixSubtractCalculator::Process(CalculatorContext* cc) {
  Matrix* subtracted = new Matrix();
  if (subtract_from_input_) {
    const Matrix& input_matrix = cc->Inputs().Tag("MINUEND").Get<Matrix>();
    const Matrix& side_input_matrix =
        cc->InputSidePackets().Tag("SUBTRAHEND").Get<Matrix>();
    if (input_matrix.rows() != side_input_matrix.rows() ||
        input_matrix.cols() != side_input_matrix.cols()) {
      return ::mediapipe::InvalidArgumentError(
          "Input matrix and the input side matrix must have the same "
          "dimension.");
    }
    *subtracted = input_matrix - side_input_matrix;
  } else {
    const Matrix& input_matrix = cc->Inputs().Tag("SUBTRAHEND").Get<Matrix>();
    const Matrix& side_input_matrix =
        cc->InputSidePackets().Tag("MINUEND").Get<Matrix>();
    if (input_matrix.rows() != side_input_matrix.rows() ||
        input_matrix.cols() != side_input_matrix.cols()) {
      return ::mediapipe::InvalidArgumentError(
          "Input matrix and the input side matrix must have the same "
          "dimension.");
    }
    *subtracted = side_input_matrix - input_matrix;
  }
  cc->Outputs().Index(0).Add(subtracted, cc->InputTimestamp());
  return ::mediapipe::OkStatus();
}

}  // namespace mediapipe