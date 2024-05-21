# Copyright (c) 2024 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from api_base import ApiBase
import paddle
import pytest
import numpy as np

test = ApiBase(
    func=paddle.shape, feed_names=["data"], feed_shapes=[[2, 4]], is_train=False
)
np.random.seed(1)


@pytest.mark.shape
@pytest.mark.filterwarnings("ignore::UserWarning")
def test_shape():
    data = np.random.uniform(1, 10, (2, 4)).astype("float32")
    test.run(feed=[data])


test_shape()