/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "custom_tag.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "bulk-send-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BulkSendApplication");

NS_OBJECT_ENSURE_REGISTERED (BulkSendApplication);

TypeId
BulkSendApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkSendApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications") 
    .AddConstructor<BulkSendApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&BulkSendApplication::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&BulkSendApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BulkSendApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BulkSendApplication::m_tid),
                   MakeTypeIdChecker ())
  ;
  return tid;
}


BulkSendApplication::BulkSendApplication ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

BulkSendApplication::~BulkSendApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
BulkSendApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
BulkSendApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
BulkSendApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void ReceivePacket (Ptr<Socket> socket)
{
  //printf("\n\n -------------- LOL ------------\n");
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
      //printf("\n\n -------------- LOL ------------\n");
    }
}

// Application Methods
void BulkSendApplication::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);

      // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
      if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
          NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.");
        }

      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }
      else if (InetSocketAddress::IsMatchingType (m_peer))
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
        }

      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&BulkSendApplication::ConnectionSucceeded, this),
        MakeCallback (&BulkSendApplication::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&BulkSendApplication::DataSend, this));

      m_socket->SetRecvCallback (MakeCallback(&ReceivePacket));
    }
  if (m_connected)
    {
      SendData ();
    }
}

void BulkSendApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("BulkSendApplication found null socket to close in StopApplication");
    }
}


// Private helpers

void BulkSendApplication::SendData (void)
{
  NS_LOG_FUNCTION (this);


  printf(" *************** BulkSend -> %d %d\n", m_maxBytes, m_totBytes);


  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more

      // uint64_t to allow the comparison later.
      // the result is in a uint32_t range anyway, because
      // m_sendSize is uint32_t.
      uint64_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (toSend, m_maxBytes - m_totBytes);
        }

      NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
      printf("sending packet at  %" PRIu64 "ns or %fs\n", Simulator::Now().GetNanoSeconds(), Simulator::Now().GetSeconds());
      Ptr<Packet> packet = Create<Packet> (toSend);
      CustomDataTag tag;
      

      InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (m_peer);
      tag.SetReceivingNode (iaddr.GetIpv4().Get());
      tag.SetPersonalTag (0);
      Address addr;

      m_socket->GetSockName (addr);
      InetSocketAddress iaddr2 = InetSocketAddress::ConvertFrom (addr);
      tag.SetNodeId (iaddr2.GetIpv4().Get());
      //timestamp is set in the default constructor of the CustomDataTag class as Simulator::Now()
      
      //attach the tag to the packet
      packet->AddByteTag (tag);

       CustomDataTag tag2;
      if (packet->FindFirstMatchingByteTag (tag2))
      {

         
          
          //printf("*******TAG IS %d to %d - %d*******\n", iaddr2.GetIpv4().Get(), tag2.GetReceivingNode(), tag2.GetPersonalTag());

          
          //InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (m_peer);
          //printf("COPIUM IS %d %s\n",  iaddr.GetIpv4().Get(), packet->ToString().c_str());
          
      }
      //std::cout << packet->ToString() << std::endl;

      int actual = m_socket->Send (packet);
      if (actual > 0)
        {
          m_totBytes += actual;
          m_txTrace (packet);
        }
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed ip.
      if ((unsigned)actual != toSend)
        {
          break;
        }
    }
  // Check if time to close (all sent)
  if (m_totBytes == m_maxBytes && m_connected && end_conn)
    {
      printf(" *************** CLosing conn\n");
      m_socket->Close ();
      m_connected = false;
    }
}

void BulkSendApplication::termiante_conn() {
  if (m_totBytes == m_maxBytes) {
    printf("Closing connection\n");
    m_socket->Close ();
    m_connected = false;
  }
}


void BulkSendApplication::prepare_new_send() {
  m_totBytes = 0;
}

void BulkSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("BulkSendApplication Connection succeeded");
  m_connected = true;
  SendData ();
}

void BulkSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("BulkSendApplication, Connection Failed");
}

void BulkSendApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);

  if (m_connected)
    { // Only send new data if the connection has completed
      SendData ();
    }
}



} // Namespace ns3
