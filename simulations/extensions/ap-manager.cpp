/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ap-manager.hpp"

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (APManager);

NS_LOG_COMPONENT_DEFINE("APManager");

TypeId 
APManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::APManager")
    .SetParent<Object> ()
    .AddConstructor<APManager>()
    ;
  return tid;
}

APManager::APManager ()
{
}

} // namespace ndn
} // namespace ns3
