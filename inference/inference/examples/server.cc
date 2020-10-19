#include "wav2letter.grpc.pb.h"
#include <iostream>
#include <memory>
#include <string>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "server.h"

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

#include <fstream>
#include <vector>

#include "inference/decoder/Decoder.h"
#include "inference/examples/AudioToWords.h"

using namespace w2l;
using namespace w2l::streaming;

struct processing_data a_data;

class echo_bytestreamImpl final : public echo_bytestream::Service {
  Status Search(ServerContext* context,
                 ServerReaderWriter<Trans_Stream , Byte_Stream>* stream) override {
  
  cout<<"asdad";
  
  

  Byte_Stream data;
  while (stream->Read(&data))
  {
    if(map_multiclient.empty())
    {
      vector<struct processing_data> received_data;
      
      a_data.data=data;
      a_data.decoder = a_data.audio_processor.initialise();

      received_data.push_back(a_data);
      map_multiclient.insert({data.unique_id(),received_data});
    }
    else
    {
      map<long,vector<struct processing_data>>::iterator it=map_multiclient.find(data.unique_id());
      if(it!=map_multiclient.end())
      {
        it->second.push_back(a_data);
        if(data.eos()==true)
        {
          map_multiclient.erase(it);
          a_data.audio_processor.destroy(a_data.decoder);
        }
      }
      else
      {
        vector<struct processing_data> received_data;

        a_data.data=data;
        a_data.decoder = a_data.audio_processor.initialise();

        received_data.push_back(a_data);
        map_multiclient.insert({data.unique_id(),received_data});
      }
    }
    std::istringstream input_audio_file(data.bstream(),std::ios::binary);
            
    TimeElapsedReporter feturesLoadingElapsed(
        "converting audio input file stream to text...");

    struct transcription t = a_data.audio_processor.process(
        input_audio_file,
        std::cout,
        a_data.decoder);

    Trans_Stream res_data;
    res_data.set_tstream(t.str);
    res_data.set_start(t.start);
    res_data.set_end(t.end);
    stream->Write(res_data);

  } 
  return Status::OK;
}
private:
  map<long,vector<struct processing_data>> map_multiclient;
};

void RunServer(int argc,char* argv[]) {
  string server_address("0.0.0.0:50051");
  echo_bytestreamImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  unique_ptr<Server> server(builder.BuildAndStart());
  cout << "Server listening on " << server_address << endl;

  a_data.audio_processor=audio_processing(argc,argv);

  server->Wait();
}

int main(int argc, char* argv[]) {
  RunServer(argc,argv);

  return 0;
}