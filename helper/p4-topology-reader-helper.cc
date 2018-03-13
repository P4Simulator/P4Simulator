/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#include "ns3/object.h"
#include "ns3/p4-topology-reader-helper.h"
#include "ns3/csma-topology-reader.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4TopologyReaderHelper");

P4TopologyReaderHelper::P4TopologyReaderHelper ()
{
  m_inputModel = 0;
}

void
P4TopologyReaderHelper::SetFileName (const std::string fileName)
{
  m_fileName = fileName;
}

void
P4TopologyReaderHelper::SetFileType (const std::string fileType)
{
  m_fileType = fileType;
}

Ptr<P4TopologyReader>
P4TopologyReaderHelper::GetTopologyReader ()
{
  if (!m_inputModel)
    {
      NS_ASSERT_MSG (!m_fileType.empty (), "Missing File Type");
      NS_ASSERT_MSG (!m_fileName.empty (), "Missing File Name");

      if (m_fileType == "CsmaTopo")
        {
          NS_LOG_INFO ("Creating Csma formatted data input.");
          m_inputModel = CreateObject<CsmaTopologyReader> ();
        }
      m_inputModel->SetFileName (m_fileName);
    }
  return m_inputModel;
}

} // namespace ns3
