#include "wav2letter.grpc.pb.h"
#include <iostream>
#include <memory>
#include <string>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include <thread>



using wav2letter::Byte_Stream;
using wav2letter::Trans_Stream;
using wav2letter::echo_bytestream;
using namespace std;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::ResourceQuota;



#include <fstream>
#include <vector>

#include "inference/decoder/Decoder.h"
#include "inference/examples/AudioToWords.h"

using namespace w2l;
using namespace w2l::streaming;

DEFINE_string(
    input_files_base_path,
    ".",
    "path is added as prefix to input files unless the input file"
    " is a full path.");
DEFINE_string(
    feature_module_file,
    "feature_extractor.bin",
    "serialized feature extraction module.");
DEFINE_string(
    acoustic_module_file,
    "acoustic_model.bin",
    "binary file containing acoustic module parameters.");
DEFINE_string(
    transitions_file,
    "",
    "binary file containing ASG criterion transition parameters.");
DEFINE_string(tokens_file, "tokens.txt", "text file containing tokens.");
DEFINE_string(lexicon_file, "lexicon.txt", "text file containing lexicon.");
DEFINE_string(silence_token, "_", "the token to use to denote silence");
DEFINE_string(
    language_model_file,
    "language_model.bin",
    "binary file containing language module parameters.");
DEFINE_string(
    decoder_options_file,
    "decoder_options.json",
    "JSON file containing decoder options"
    " including: max overall beam size, max beam for token selection, beam score threshold"
    ", language model weight, word insertion score, unknown word insertion score"
    ", silence insertion score, and use logadd when merging decoder nodes");

std::string GetInputFileFullPath(const std::string& fileName) {
  return GetFullPath(fileName, FLAGS_input_files_base_path);
}

struct processing_data
{
  vector<wav2letter::Byte_Stream> data;
  Trans_Stream *res_data;
  audio_processing* audio_processor;
  w2l::streaming::Decoder *decoder;
  std::shared_ptr<w2l::streaming::Sequential> dnnModule = std::make_shared<streaming::Sequential>();
};

struct processing_data *a_data;

int nTokens ;
std::vector<std::string> tokens;
std::shared_ptr<streaming::Sequential> featureModule;
std::shared_ptr<streaming::Sequential> acousticModule;
std::vector<float> transitions;
std::shared_ptr<const DecoderFactory> decoderFactory;
struct w2l::DecoderOptions decoderOptions;

class echo_bytestreamImpl final : public echo_bytestream::Service {
  Status Search(ServerContext* context,
                 ServerReaderWriter<Trans_Stream , Byte_Stream>* stream) override {

  Byte_Stream data;
  while (stream->Read(&data))
  {
    map<long,struct processing_data*>::iterator it=map_multiclient.find(data.unique_id());
    if(it!=map_multiclient.end())
    {
      it->second->data.push_back(data);
      a_data=it->second;

    }
    else
    {
      a_data = new processing_data();

      a_data->data.push_back(data);
      a_data->audio_processor= new audio_processing();
      a_data->dnnModule->add(featureModule);
      a_data->dnnModule->add(acousticModule);
      a_data->decoder = a_data->audio_processor->initialise(decoderFactory,decoderOptions,a_data->dnnModule);

      map_multiclient.insert({data.unique_id(),a_data});
    }
    

    
    //TimeElapsedReporter feturesLoadingElapsed(
        //"converting audio input file stream to text...");
    
    std::istringstream input_audio_file(data.bstream(),std::ios::binary);

    struct transcription t = a_data->audio_processor->process(
        input_audio_file,
        std::cout,
        a_data->decoder,
        a_data->dnnModule,
        nTokens);

    cout<<"0"<<endl;  
    
    a_data->res_data=new Trans_Stream();
    a_data->res_data->set_tstream(t.str);
    a_data->res_data->set_start(t.start);
    a_data->res_data->set_end(t.end);
    stream->Write(*(a_data->res_data));
    
    delete a_data->res_data;

    if(data.eos()==true)
    {
      cout<<"delete"<<endl;
      it->second->audio_processor->destroy(it->second->decoder,it->second->dnnModule);
      delete it->second->audio_processor;
      delete it->second;
      map_multiclient.erase(it);
    }
  } 
  return Status::OK;
}
private:
  map<long,struct processing_data*> map_multiclient;
};

void RunServer(int argc,char* argv[]) {
  string server_address("0.0.0.0:50051");
  echo_bytestreamImpl service;
  
  //ResourceQuota rq;
  //rq.SetMaxThreads(5);
  
  ServerBuilder builder;
  //builder.SetResourceQuota(rq);
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  unique_ptr<Server> server(builder.BuildAndStart());
  cout << "Server listening on " << server_address << endl;
  
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Read files
    {
      TimeElapsedReporter feturesLoadingElapsed("features model file loading");
      std::ifstream featFile(
          GetInputFileFullPath(FLAGS_feature_module_file), std::ios::binary);
      if (!featFile.is_open()) {
        throw std::runtime_error(
            "failed to open feature file=" +
            GetInputFileFullPath(FLAGS_feature_module_file) + " for reading");
      }
      cereal::BinaryInputArchive ar(featFile);
      ar(featureModule);
    }

    {
      TimeElapsedReporter acousticLoadingElapsed("acoustic model file loading");
      std::ifstream amFile(
          GetInputFileFullPath(FLAGS_acoustic_module_file), std::ios::binary);
      if (!amFile.is_open()) {
        throw std::runtime_error(
            "failed to open acoustic model file=" +
            GetInputFileFullPath(FLAGS_feature_module_file) + " for reading");
      }
      cereal::BinaryInputArchive ar(amFile);
      ar(acousticModule);
    }

    {
      TimeElapsedReporter acousticLoadingElapsed("tokens file loading");
      std::ifstream tknFile(GetInputFileFullPath(FLAGS_tokens_file));
      if (!tknFile.is_open()) {
        throw std::runtime_error(
            "failed to open tokens file=" +
            GetInputFileFullPath(FLAGS_tokens_file) + " for reading");
      }
      std::string line;
      while (std::getline(tknFile, line)) {
        tokens.push_back(line);
      }
    }
    nTokens = tokens.size();
    std::cout << "Tokens loaded - " << nTokens << " tokens" << std::endl;
  
    {
    TimeElapsedReporter decoderOptionsElapsed("decoder options file loading");
    std::ifstream decoderOptionsFile(
        GetInputFileFullPath(FLAGS_decoder_options_file));
    if (!decoderOptionsFile.is_open()) {
      throw std::runtime_error(
          "failed to open decoder options file=" +
          GetInputFileFullPath(FLAGS_decoder_options_file) + " for reading");
    }
    cereal::JSONInputArchive ar(decoderOptionsFile);
    // TODO: factor out proper serialization functionality or Cereal
    // specialization.
    ar(cereal::make_nvp("beamSize", decoderOptions.beamSize),
      cereal::make_nvp("beamSizeToken", decoderOptions.beamSizeToken),
      cereal::make_nvp("beamThreshold", decoderOptions.beamThreshold),
      cereal::make_nvp("lmWeight", decoderOptions.lmWeight),
      cereal::make_nvp("wordScore", decoderOptions.wordScore),
      cereal::make_nvp("unkScore", decoderOptions.unkScore),
      cereal::make_nvp("silScore", decoderOptions.silScore),
      cereal::make_nvp("eosScore", decoderOptions.eosScore),
      cereal::make_nvp("logAdd", decoderOptions.logAdd),
      cereal::make_nvp("criterionType", decoderOptions.criterionType));
  }

  if (!FLAGS_transitions_file.empty()) {
    TimeElapsedReporter acousticLoadingElapsed("transitions file loading");
    std::ifstream transitionsFile(
        GetInputFileFullPath(FLAGS_transitions_file), std::ios::binary);
    if (!transitionsFile.is_open()) {
      throw std::runtime_error(
          "failed to open transition parameter file=" +
          GetInputFileFullPath(FLAGS_transitions_file) + " for reading");
    }
    cereal::BinaryInputArchive ar(transitionsFile);
    ar(transitions);
  }

  // Create Decoder
  {
    TimeElapsedReporter acousticLoadingElapsed("create decoder");
    decoderFactory = std::make_shared<DecoderFactory>(
        GetInputFileFullPath(FLAGS_tokens_file),
        GetInputFileFullPath(FLAGS_lexicon_file),
        GetInputFileFullPath(FLAGS_language_model_file),
        transitions,
        SmearingMode::MAX,
        FLAGS_silence_token,
        0);
  }
  
  server->Wait();
}

int main(int argc, char* argv[]) {
  RunServer(argc,argv);

  return 0;
}