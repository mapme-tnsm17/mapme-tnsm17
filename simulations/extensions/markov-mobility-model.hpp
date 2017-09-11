#ifndef MARKOV_MOBILITY_MODEL_HPP
#define MARKOV_MOBILITY_MODEL_HPP

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-model.h"

namespace ns3 {
namespace ndn {

/**
 * \ingroup mobility
 *
 * \brief Mobility model for which nodes stay in place for an exponentially distributed duration, before jumping to the next node. Currently, node position are based on a set of predefined nodes to which they attach. This class is tightly coupled with the ConnectivityManager module.
 */
class MarkovMobilityModel: public MobilityModel
{
public:
  static TypeId GetTypeId (void);

  MarkovMobilityModel();

  //MarkovMobilityModel (NodeContainer * Nodes, double mean_sleep);

  void SetMeanSleep(double mean_sleep);
  void SetNodes(NodeContainer * Nodes);
  void SetModel(std::string model);

private:

  /*
   * \brief Update the m_current_leaf_id variable according to the mobility
   * model specified by SetModel, in m_model.
   *
   * \returns An offset in the m_nodes NodeContainer.
   */
  void Jump();

  void GetShortestPath(uint32_t src_id, uint32_t dst_id);
  uint32_t GetUniformNodeId(uint32_t num, uint32_t avoid);
  uint32_t GetLeafIdFromNodeId(uint32_t node_id);

  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;

private:
  /* Parameters */

  /** A set of leaves on which to 'attach' */
  NodeContainer * m_nodes;
  Ptr<UniformRandomVariable> m_uniform;
  Ptr<ExponentialRandomVariable> m_sleep_rv;
  std::string m_model;

  /* State */
  uint32_t m_current_leaf_id;
  std::vector<uint32_t> m_path;
  Vector m_position;
};

} // namespace ndn
} // namespace ns3

#endif /* MARKOV_MOBILITY_MODEL_HPP */
