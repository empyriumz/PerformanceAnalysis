#include<chimbuko/ad/ADProvenanceDBclient.hpp>

using namespace chimbuko;

thallium::engine ADProvenanceDBclient::m_engine("ofi+tcp", THALLIUM_CLIENT_MODE);

void ADProvenanceDBclient::connect(const std::string &addr){
  try{
    std::cout << "DB client connecting to " << addr << std::endl;
    m_database = m_client.open(addr, 0, "provdb");
    std::cout << "DB client opening anomaly collection" << std::endl;
    m_coll_anomalies = m_database.open("anomalies");
    std::cout << "DB client opening metadata collection" << std::endl;
    m_coll_metadata = m_database.open("metadata");
    m_is_connected = true;
    std::cout << "DB client connected successfully" << std::endl;
  }catch(const sonata::Exception& ex) {
    throw std::runtime_error(std::string("Provenance DB client could not connect due to exception: ") + ex.what());
  }
}

ADProvenanceDBclient::~ADProvenanceDBclient(){
  using namespace sonata;
  if(m_is_connected){
    // m_coll_anomalies.~Collection();
    // m_coll_metadata.~Collection();
    // m_database.~Database();
	
    //m_engine.finalize(); //necessary to prevent hangs on reconnection but currently causes some Margo errors that seem benign
  }
}


uint64_t ADProvenanceDBclient::sendData(const nlohmann::json &entry, const ProvenanceDataType type) const{
  if(m_is_connected){
    return getCollection(type).store(entry.dump());
  }
  return -1;
}



bool ADProvenanceDBclient::retrieveData(nlohmann::json &entry, uint64_t index, const ProvenanceDataType type) const{
  if(m_is_connected){
    std::string data;
    try{
      getCollection(type).fetch(index, &data);
    }catch(const sonata::Exception &e){
      return false;
    }
    entry = nlohmann::json::parse(data);
    return true;
  }
  return false;
}