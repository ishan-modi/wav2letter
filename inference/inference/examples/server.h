#include "inference/decoder/Decoder.h"
#include "inference/examples/AudioToWords.h"
#include "wav2letter.grpc.pb.h"

namespace w2l{
namespace streaming{

    struct processing_data
    {
    wav2letter::Byte_Stream data;
    audio_processing audio_processor;
    Decoder decoder;
    };
}
}