/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Hajime Tazaki
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
 * Author: Hajime Tazaki (tazaki@sfc.wide.ad.jp)
 *         Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#include "rocketfuel-weights-reader.h"

#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-address.h"

#include "ns3/constant-position-mobility-model.h"
#include "ns3/random-variable.h"

#include <regex.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <iomanip>
#include <set>

using namespace std;

NS_LOG_COMPONENT_DEFINE ("RocketfuelWeightsReader");

namespace ns3 {
    
RocketfuelWeightsReader::RocketfuelWeightsReader ()
{
  NS_LOG_FUNCTION (this);
}
    
RocketfuelWeightsReader::~RocketfuelWeightsReader ()
{
  NS_LOG_FUNCTION (this);
}

void
RocketfuelWeightsReader::SetFileType (uint8_t inputType)
{
  m_inputType = inputType;
}

NodeContainer
RocketfuelWeightsReader::Read ()
{
  ifstream topgen;
  topgen.open (GetFileName ().c_str ());
  NodeContainer nodes;
        
  if ( !topgen.is_open () )
    {
      NS_LOG_ERROR ("Cannot open file " << GetFileName () << " for reading");
      return nodes;
    }

  map<string, set<string> > processedLinks; // to eliminate duplications
  bool repeatedRun = LinksSize () > 0;
  std::list<Link>::iterator linkIterator = m_linksList.begin ();
  
  while (!topgen.eof ())
    {
      string line;
      getline (topgen,line);
      if (line == "") continue;
      if (line[0] == '#') continue; // comments

      // NS_LOG_DEBUG ("Input: [" << line << "]");
      
      istringstream lineBuffer (line);
      string from, to, attribute;

      lineBuffer >> from >> to >> attribute;

      if (processedLinks[to].size () != 0 &&
          processedLinks[to].find (from) != processedLinks[to].end ())
        {
          continue; // duplicated link
        }
      processedLinks[from].insert (to);
      
      Ptr<Node> fromNode = Names::Find<Node> (m_path, from);
      if (fromNode == 0)
        {
          fromNode = CreateNode (from);
          nodes.Add (fromNode);
        }

      Ptr<Node> toNode   = Names::Find<Node> (m_path, to);
      if (toNode == 0)
        {
          toNode = CreateNode (to);
          nodes.Add (toNode);
        }

      Link *link;
      if (!repeatedRun)
        link = new Link (fromNode, from, toNode, to);
      else
        {
          NS_ASSERT (linkIterator != m_linksList.end ());
          link = &(*linkIterator);

          linkIterator++;
        }

      switch (m_inputType)
        {
        case WEIGHTS:
          link->SetAttribute ("OSPF", attribute);
          break;
        case LATENCIES:
          link->SetAttribute ("Delay", attribute);
          break;
        default:
          ; //
        }

      NS_LOG_DEBUG ("Link " << from << " <==> " << to << " / " << attribute);
      if (!repeatedRun)
        {
          AddLink (*link);
          delete link;
        }
    }
        
  topgen.close ();
  NS_LOG_INFO ("Rocketfuel topology created with " << nodes.GetN () << " nodes and " << LinksSize () << " links");
  return nodes;
}
    
} /* namespace ns3 */