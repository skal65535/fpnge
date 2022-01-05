// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <thread>
#include <vector>

#include "fpnge.h"
#include "lodepng.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s in.png out.jxl [num_reps]\n", argv[0]);
    return 1;
  }

  const char *in = argv[1];
  const char *out = argv[2];
  size_t num_reps = argc >= 4 ? atoi(argv[3]) : 0;

  FILE *infile = fopen(in, "rb");
  if (!infile) {
    fprintf(stderr, "error opening %s: %s\n", in, strerror(errno));
    return 1;
  }
  fseek(infile, 0, SEEK_END);
  std::vector<unsigned char> in_data(ftell(infile));
  fseek(infile, 0, SEEK_SET);
  if (fread(in_data.data(), 1, in_data.size(), infile) != in_data.size()) {
    fprintf(stderr, "error reading from %s: %s\n", in, strerror(errno));
    exit(1);
  }

  unsigned char *png;
  unsigned width, height;

  // RGB only for now.
  unsigned error =
      lodepng_decode24(&png, &width, &height, in_data.data(), in_data.size());

  if (error) {
    fprintf(stderr, "lodepng error %u: %s\n", error, lodepng_error_text(error));
    return 1;
  }

  size_t encoded_size = 0;
  unsigned char *encoded = nullptr;

  if (num_reps > 0) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t _ = 0; _ < num_reps; _++) {
      free(encoded);
      encoded_size = FPNGEEncode(1, 3, png, width, width * 3, height, &encoded);
    }
    auto stop = std::chrono::high_resolution_clock::now();
    float us =
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start)
            .count();
    size_t pixels = size_t{width} * size_t{height} * num_reps;
    float mps = pixels / us;
    fprintf(stderr, "%10.3f MP/s\n", mps);
    fprintf(stderr, "%10.3f bits/pixel\n",
            encoded_size * 8.0 / float(width) / float(height));
  } else {
    encoded_size = FPNGEEncode(1, 3, png, width, width * 3, height, &encoded);
  }

  FILE *o = fopen(out, "wb");
  if (!o) {
    fprintf(stderr, "error opening %s: %s\n", out, strerror(errno));
    return 1;
  }
  if (fwrite(encoded, 1, encoded_size, o) != encoded_size) {
    fprintf(stderr, "error writing to %s: %s\n", out, strerror(errno));
  }
  fclose(o);
  free(png);
  free(encoded);
}
