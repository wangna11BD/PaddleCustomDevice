// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "kernels/funcs/npu_funcs.h"
#include "kernels/funcs/npu_op_runner.h"

namespace custom_kernel {

template <typename T, typename Context>
void AclopPadKernel(const Context& dev_ctx,
                    const phi::DenseTensor& x,
                    const std::vector<int>& paddings,
                    const phi::Scalar& pad_value_scalar,
                    phi::DenseTensor* out) {
  dev_ctx.template Alloc<T>(out);
  auto stream = dev_ctx.stream();

  auto pad_value = pad_value_scalar.to<float>();

  PADDLE_ENFORCE_LT(
      abs(pad_value),
      1e-5,
      phi::errors::Unimplemented("npu npu only support pad_value=0 right now,"
                                 "but received pad_value is %f .",
                                 pad_value));

  NpuOpRunner runner;
  runner.SetType("Pad")
      .AddInput(x)
      .AddInput(dev_ctx, std::move(paddings))
      .AddOutput(*out);

  runner.Run(stream);
}

template <typename T, typename Context>
void PadKernel(const Context& dev_ctx,
               const phi::DenseTensor& x,
               const std::vector<int>& paddings,
               const phi::Scalar& pad_value_scalar,
               phi::DenseTensor* out) {
  DO_COMPATIBILITY(aclnnConstantPadNd,
                   (custom_kernel::AclopPadKernel<T, Context>(
                       dev_ctx, x, paddings, pad_value_scalar, out)));
  dev_ctx.template Alloc<T>(out);
  auto stream = dev_ctx.stream();
  auto pad_value = pad_value_scalar.to<float>();
  PADDLE_ENFORCE_LT(
      abs(pad_value),
      1e-5,
      phi::errors::Unimplemented("npu npu only support pad_value=0 right now,"
                                 "but received pad_value is %f .",
                                 pad_value));
  std::vector<int64_t> paddings_;
  for (size_t i = 0; i < paddings.size(); ++i) {
    paddings_.push_back(paddings[i]);
  }
  static const auto aclCreateIntArray = GET_OP_API_FUNC(aclCreateIntArray);
  auto paddings_acl = aclCreateIntArray(paddings_.data(), paddings_.size());
  phi::Scalar value = 0;
  EXEC_NPU_CMD(aclnnConstantPadNd, dev_ctx, x, paddings_acl, value, *out);
}

template <typename T, typename Context>
void AclopPadGradKernel(const Context& dev_ctx,
                        const phi::DenseTensor& dout,
                        const std::vector<int>& paddings,
                        const phi::Scalar& pad_value,
                        phi::DenseTensor* dx) {
  dev_ctx.template Alloc<T>(dx);
  auto stream = dev_ctx.stream();

  auto d_x_dims = dx->dims();
  auto size = phi::vectorize(d_x_dims);
  std::vector<int> offsets(0);
  int i = 0;
  for (auto iter = paddings.begin(); iter < paddings.end(); ++iter, ++i) {
    if (i % 2 == 0) {
      offsets.push_back(*iter);
    }
  }

  NpuOpRunner runner;
  runner.SetType("Slice")
      .AddInput(dout)
      .AddInput(dev_ctx, std::move(offsets))
      .AddInput(dev_ctx, std::move(size))
      .AddOutput(*dx)
      .Run(stream);
}

template <typename T, typename Context>
void PadGradKernel(const Context& dev_ctx,
                   const phi::DenseTensor& dout,
                   const std::vector<int>& paddings,
                   const phi::Scalar& pad_value,
                   phi::DenseTensor* dx) {
  DO_COMPATIBILITY(aclnnSliceV2,
                   (custom_kernel::AclopPadGradKernel<T, Context>(
                       dev_ctx, dout, paddings, pad_value, dx)));
  dev_ctx.template Alloc<T>(dx);
  auto stream = dev_ctx.stream();

  auto d_x_dims = dx->dims();
  auto size = phi::vectorize(d_x_dims);
  std::vector<int64_t> offsets(0);
  int64_t i = 0;
  for (auto iter = paddings.begin(); iter < paddings.end(); ++iter, ++i) {
    if (i % 2 == 0) {
      offsets.push_back(*iter);
    }
  }

  std::vector<int64_t> axes;
  for (int i = 0; i < dout.dims().size(); i++) {
    axes.push_back(i);
  }
  std::vector<int64_t> steps(axes.size(), 1);
  EXEC_NPU_CMD(aclnnSliceV2, dev_ctx, dout, offsets, size, axes, steps, *dx);
}

}  // namespace custom_kernel

PD_REGISTER_PLUGIN_KERNEL(pad,
                          npu,
                          ALL_LAYOUT,
                          custom_kernel::PadKernel,
                          int16_t,
                          int32_t,
                          int64_t,
                          float,
                          phi::dtype::float16,
                          phi::dtype::bfloat16,
                          double) {}

PD_REGISTER_PLUGIN_KERNEL(pad_grad,
                          npu,
                          ALL_LAYOUT,
                          custom_kernel::PadGradKernel,
                          int16_t,
                          int32_t,
                          int64_t,
                          float,
                          phi::dtype::float16,
                          phi::dtype::bfloat16,
                          double) {}
