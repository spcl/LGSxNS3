/*
 * cacc-data-tag.cc
 *
 *  Created on: Oct 29, 2019
 *      Author: adil
 */
#include "custom_tag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("CustomDataTag");
NS_OBJECT_ENSURE_REGISTERED (CustomDataTag);

CustomDataTag::CustomDataTag() {
	m_timestamp = Simulator::Now();
	m_nodeId = -1;
}
CustomDataTag::CustomDataTag(uint32_t node_id) {
	m_timestamp = Simulator::Now();
	m_nodeId = node_id;
}

CustomDataTag::~CustomDataTag() {
}

//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
TypeId CustomDataTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomDataTag")
    .SetParent<Tag> ()
    .AddConstructor<CustomDataTag> ();
  return tid;
}
TypeId CustomDataTag::GetInstanceTypeId (void) const
{
  return CustomDataTag::GetTypeId ();
}

/** The size required for the data contained within tag is:
 *   	size needed for a ns3::Vector for position +
 * 		size needed for a ns3::Time for timestamp + 
 * 		size needed for a uint32_t for node id
 */
uint32_t CustomDataTag::GetSerializedSize (void) const
{
	return sizeof (ns3::Time) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) ;
}
/**
 * The order of how you do Serialize() should match the order of Deserialize()
 */
void CustomDataTag::Serialize (TagBuffer i) const
{
	//we store timestamp first
	i.WriteDouble(m_timestamp.GetDouble());

	//Then we store the node ID
	i.WriteU32(m_nodeId);
	i.WriteU32(receiving_node);
	i.WriteU32(personal_tag);
	i.WriteU32(offset);
	i.WriteU32(current_hop);	
}
/** This function reads data from a buffer and store it in class's instance variables.
 */
void CustomDataTag::Deserialize (TagBuffer i)
{
	//We extract what we stored first, so we extract the timestamp
	m_timestamp =  Time::FromDouble (i.ReadDouble(), Time::NS);;

	//Finally, we extract the node id
	m_nodeId = i.ReadU32();
	receiving_node = i.ReadU32();
	personal_tag = i.ReadU32();
	offset = i.ReadU32();
	current_hop = i.ReadU32();

}
/**
 * This function can be used with ASCII traces if enabled. 
 */
void CustomDataTag::Print (std::ostream &os) const
{
  os << "Custom Data --- Node :" << m_nodeId <<  "\t(" << m_timestamp  << ")";
}

//Your accessor and mutator functions 
uint32_t CustomDataTag::GetNodeId() {
	return m_nodeId;
}

//Your accessor and mutator functions 
uint32_t CustomDataTag::GetReceivingNode() {
	return receiving_node;
}

//Your accessor and mutator functions 
uint32_t CustomDataTag::GetPersonalTag() {
	return personal_tag;
}

void CustomDataTag::SetNodeId(uint32_t node_id) {
	m_nodeId = node_id;
}

void CustomDataTag::SetPersonalTag(uint32_t tag) {
	personal_tag = tag;
}

void CustomDataTag::SetReceivingNode(uint32_t node_id) {
	receiving_node = node_id;
}

Time CustomDataTag::GetTimestamp() {
	return m_timestamp;
}

void CustomDataTag::SetOffset (uint32_t size) {
	offset = size;
}

uint32_t CustomDataTag::GetOffset() {
	return offset;
}

void CustomDataTag::SetTimestamp(Time t) {
	m_timestamp = t;
}

uint32_t CustomDataTag::GetCurrentHop() {
	return current_hop;
}

uint32_t* CustomDataTag::GetHops() {
	return hop;
}

void CustomDataTag::SetCurrentHop(uint32_t my_hop) {
	current_hop = my_hop;
}

void CustomDataTag::SetHops(uint32_t value) {
	hop[current_hop] = value;
}
} /* namespace ns3 */

