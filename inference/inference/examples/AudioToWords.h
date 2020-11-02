/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "inference/decoder/Decoder.h"
#include "inference/module/nn/nn.h"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <gflags/gflags.h>

#include "inference/decoder/Decoder.h"
#include "inference/examples/AudioToWords.h"
#include "inference/examples/Util.h"
#include "inference/module/feature/feature.h"
#include "inference/module/module.h"
#include "inference/module/nn/nn.h"


namespace w2l {
namespace streaming {

// @inputAudioStream is a 16KHz wav file.
void audioStreamToWordsStream(
    std::istream& inputAudioStream,
    std::ostream& outputWordsStream,
    std::shared_ptr<streaming::Sequential> dnnModule,
    std::shared_ptr<const DecoderFactory> decoderFactory,
    const DecoderOptions& decoderOptions,
    int nTokens);

// @inputFileName is a 16KHz wav file.
// @errorStream file errors are written to errorStream.
void audioFileToWordsFile(
    const std::string& inputFileName,
    const std::string& outputFileName,
    std::shared_ptr<streaming::Sequential> dnnModule,
    std::shared_ptr<const DecoderFactory> decoderFactory,
    const DecoderOptions& decoderOptions,
    int nTokens,
    std::ostream& errorStream);

// @inputFileName is a 16KHz wav file.
// Errors are throws as exceptions.
void audioFileToWordsFile(
    const std::string& inputFileName,
    const std::string& outputFileName,
    std::shared_ptr<streaming::Sequential> dnnModule,
    std::shared_ptr<const DecoderFactory> decoderFactory,
    const DecoderOptions& decoderOptions,
    int nTokens);

struct transcription
{
  std::string str;
  long start;
  long end;
};

class audio_processing
{
public:
  Decoder* initialise(std::shared_ptr<const DecoderFactory> decoderFactory,
                      struct w2l::DecoderOptions decoderOptions,
                      std::shared_ptr<w2l::streaming::Sequential> dnnModule);

  void destroy(w2l::streaming::Decoder* decoder,
              std::shared_ptr<w2l::streaming::Sequential> dnnModule);

  struct transcription process(
    std::istream& inputAudioStream,
    std::ostream& outputWordsStream,
    Decoder* decoder,
    std::shared_ptr<w2l::streaming::Sequential> dnnModule,
    int nTokens);

private:
  std::shared_ptr<w2l::streaming::IOBuffer> inputBuffer;
  std::shared_ptr<w2l::streaming::IOBuffer> outputBuffer;
  std::shared_ptr<w2l::streaming::ModuleProcessingState> input ;
  int audioSampleCount;

};

} // namespace streaming
} // namespace w2l
