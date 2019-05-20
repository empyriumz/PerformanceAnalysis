#include "chimbuko/net.hpp"
#include "chimbuko/net/mpi_net.hpp"
#include "chimbuko/net/zmq_net.hpp"

#include <iostream>

using namespace chimbuko;

NetInterface& DefaultNetInterface::get()
{
#ifdef _USE_MPINET
    static MPINet net;
    return net;
#else
    static ZMQNet net;
    return net;
#endif
}

NetInterface::NetInterface() : m_nt(0), m_param(nullptr), m_stop(false)
{
}

NetInterface::~NetInterface()
{

}

void NetInterface::set_parameter(ParamInterface* param)
{
    //m_kind = kind;
    m_param = param;
}

