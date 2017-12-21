/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 *
 *
 * The source code of this software has been developed by:
 * Karlsruhe Institute of Technology
 * Institute of Telematics
 * Zirkel 2, 76131 Karlsruhe
 * Germany
 *
 * with contributions from some individuals, listed in 'AUTHORS.txt'.
 */


#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <unordered_map>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/seed-parser.h"
#include "ns3/seed-node.h"
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/tap-bridge-module.h>
#include <ns3/csma-module.h>

using namespace rapidxml;
using namespace std;
using namespace ns3;
using namespace seed;

NS_LOG_COMPONENT_DEFINE ("SEED");

/* print simulation time for more verbosity */
void
printCurrentSimulationTime(uint32_t runtime) {
    double percentage = Simulator::Now ().GetSeconds () / runtime * 100;
    // NS_LOG_INFO ("  " << Simulator::Now ().GetSeconds () << "/"  << runtime << "s (" << percentage << "%)");
    cout <<  "  " << Simulator::Now ().GetSeconds () << "/"  << runtime << "s (" << percentage << "%)" << endl;
    Simulator::Schedule (Seconds (5), &printCurrentSimulationTime, runtime);
}



    int
main (int argc, char *argv[])
{
    // LogComponentEnable("SEED", LOG_LEVEL_ALL);
    // LogComponentEnable("AnimationInterface", LOG_LEVEL_DEBUG);

    // OFSwitch13Helper::EnableDatapathLogs ();
    // LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
    // LogComponentEnable ("OFSwitch13ExternalHelper", LOG_LEVEL_ALL);


    NS_LOG_INFO ("STARTUP");
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: STARTING..." << endl;

    // Enable checksum computations (required by OFSwitch13 module)
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    cout << " => Enabling Checksums" << endl;
    // Set simulator to real time mode
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    cout << " => Enabling ns3 Realtime-Mode" << endl;


    // Now, create random variables
    // Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    // Ptr<ExponentialRandomVariable> y = CreateObject<ExponentialRandomVarlable> ();

    uint32_t runTime = 60;
    string bundleFile = "seed-ressources/prototype.xml";
    string bindingFile = "seed-ressources/prototype.controller-bindings.yml";
    string loggingPath = "/home/ns3/result";
    string loggingName = "seed";
    uint32_t randomSeed = 1337;
    uint32_t randomRun = 42;
    bool pcapEnabled = true;
    bool switchFallback = false;

    // CLI Params
    CommandLine cmd;
    cmd.AddValue ("bundle", "Path to bundle xml-file", bundleFile);
    cmd.AddValue ("binding", "Path to binding yml-file", bindingFile);
    cmd.AddValue ("runtime", "Runtime of simulation", runTime);
    cmd.AddValue ("pcap", "Enable PCAP", pcapEnabled);
    cmd.AddValue ("no-sdn", "Use generic switches instead of SDN-switches", switchFallback);
    cmd.AddValue ("randomSeed", "Seed for generation of randomness", randomSeed);
    cmd.AddValue ("randomRun", "Run for generation of randomness", randomRun);
    cmd.AddValue ("o", "Path of logfiles", loggingPath);
    cmd.AddValue ("n", "Name for logfiles", loggingName);
    cmd.Parse (argc, argv);


    cout << " => Initializing random-seed with " << randomSeed << endl;
    RngSeedManager::SetSeed (randomSeed);
    cout << " => Initializing random-run with " << randomRun << endl;
    RngSeedManager::SetRun (randomRun);


    // Start XML Parsing
    cout << endl;
    cout << "  ==> Parsing bundle file: \"" << bundleFile.c_str() << "\"" << endl;
    SeedParser seedParser = SeedParser( bundleFile.c_str() );
    seedParser.parse();



    /////////////////////////////////////////////////////////////////////
    // ACTUAL SETUP
    /////////////////////////////////////////////////////////////////////
    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: Creating Topology" << endl;


    // ACTUAL NODE CREATION
    cout << "  => Creating Nodes & Interfaces" << endl;
    NodeContainer allNodesContainer;
    NodeContainer allHostNodesContainer;
    NodeContainer allGenericSwitchContainer;
    NodeContainer allOfSwitchContainer;
    NodeContainer allOfControllerContainer;
    NetDeviceContainer allInterfacesContainer;
    NetDeviceContainer ofControllerInterfaces;
    NetDeviceContainer allOfControllerInterfaces;

    allNodesContainer.Create (seedParser.getNodes().size());
    uint32_t nodeIndex = 0;
    uint32_t interfaceIndex = 0;
    auto allSeedNodes = seedParser.getNodes();
    unordered_map<string, SeedInterface> allSeedInterfaces;
    for(auto &it : allSeedNodes)
    {
        auto sn = &it.second;
        sn->setIndex(nodeIndex);

        cout << "  ==> Configuring node \"" << sn->getName() << "\"" << endl;

        // creating interfaces for this node
        NetDeviceContainer nodesInterfaces;
        for(auto &si : sn->getInterfaces())
        {
            si->setIndex(interfaceIndex);
            cout << "    --> with Interface: " << si->getName() << endl;
            allSeedInterfaces.insert(std::make_pair(si->getName(), *si));
            nodesInterfaces.Add ( si->getName() );
            interfaceIndex++;
        }
        allInterfacesContainer.Add (nodesInterfaces);

        switch(sn->getType())
        {
            case NodeType::HOST:
                allHostNodesContainer.Add (allNodesContainer.Get (sn->getIndex() ));
                break;
            case NodeType::GENERIC_SWITCH:
                allGenericSwitchContainer.Add (allNodesContainer.Get (sn->getIndex() ));
                break;
            case NodeType::OF_SWITCH:
                allOfSwitchContainer.Add (allNodesContainer.Get (sn->getIndex() ));
                break;
            case NodeType::OF_CONTROLLER:
                //FIXME multiple controllers!
                allOfControllerContainer.Add (allNodesContainer.Get (sn->getIndex() ));
                break;
            case NodeType::UNDEFINED:
            default:
                cout << "Error" << endl;
                break;
        }


        nodeIndex++;
    }



    // ACTUAL LINK CREATIION + WIRING
    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: Stack Setup" << endl;
    cout << "  => Deploying Internet-Stack (on hosts & generic-switches)" << endl;
    uint16_t linkIndex = 0;
    Ipv4InterfaceContainer allIpAddresses;
    InternetStackHelper internet;
    // FIXME
    // internet.Install (allNodesContainer);
    internet.Install (allHostNodesContainer);
    internet.Install (allGenericSwitchContainer);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.13.1.0", "255.255.255.0");







    Ptr<OFSwitch13ExternalHelper> of13Helper = CreateObject<OFSwitch13ExternalHelper> ();
    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: SDN Setup" << endl;
    cout << "  ==> Creating External Controllers & bridging ControllerNode to external TapDevice" << endl;
    Ptr<NetDevice> ctrlDev;
    Ptr<Node> ctrlNode;
    uint32_t ctrlIndex = 0;
    for(auto &nodeIt : allSeedNodes)
    {
        auto sn = &nodeIt.second;

        if(sn->getType() == NodeType::OF_CONTROLLER){
            Ptr<OFSwitch13ExternalHelper> of13Helper = CreateObject<OFSwitch13ExternalHelper> ();
            uint32_t portToUse = 6653 + ctrlIndex;
            cout << "    --> Configuring Controller \"" << sn->getName() << "\"" << endl;
            cout << "      --> with external device \"" << sn->getControllerName() << "\" @ port " << portToUse  << endl;
            ctrlNode = allNodesContainer.Get(sn->getIndex());
            of13Helper->SetAttribute("Port", UintegerValue(portToUse));
            ctrlDev = of13Helper->InstallExternalController (allNodesContainer.Get(sn->getIndex()));
            TapBridgeHelper tapBridge;
            tapBridge.SetAttribute ("Mode", StringValue ("ConfigureLocal"));
            // tapBridge.SetAttribute ("Mode", StringValue ("UseBridge"));
            // tapBridge.SetAttribute ("DeviceName", StringValue ("ctrl"));
            tapBridge.SetAttribute ("DeviceName", StringValue (sn->getControllerName()));
            tapBridge.Install (allNodesContainer.Get(sn->getIndex()), ctrlDev);
            ctrlIndex++;
        }
    }

    cout << "  ==> Wiring OF-Switches with OF-Controllers" << endl;
    for(auto &nodeIt : allSeedNodes)
    {
        auto sn = &nodeIt.second;

        std::string linkName = "";
        std::string bandwidth = "";
        std::string delay = "";

        if(sn->getType() == NodeType::OF_SWITCH){
            bool linkFound = false;
            for(auto &it : seedParser.getLinks())
            {
                auto sl = &it.second;

                for(auto &si : sn->getInterfaces())
                {
                    if(si->getType() == "control"
                            && (si->getName() == sl->getSource() || si->getName() == sl->getDestination() )
                      )
                    {
                        ctrlNode = allNodesContainer.Get(sn->getIndex());
                        linkFound = true;
                        linkName = sl->getName();
                        bandwidth = sl->getBandwidth();
                        delay = sl->getDelay();
                    }
                }
            }

            if(linkFound)
            {
                Ptr<OFSwitch13ExternalHelper> of13Helper = CreateObject<OFSwitch13ExternalHelper> ();
                CsmaHelper csmaHelper;
                csmaHelper.SetChannelAttribute ("DataRate", StringValue ( bandwidth ));
                csmaHelper.SetChannelAttribute ("Delay", StringValue ( delay ));

                Ptr<Node> switchNode = allNodesContainer.Get(sn->getIndex());
                cout << "    --> Wiring OF-Switch \"" << sn->getName() << "\" via link \"" << linkName << "\"" << endl;

                NodeContainer pair;
                NetDeviceContainer pairDevs;
                NetDeviceContainer switchPorts;
                // pair = NodeContainer (switchNode, ctrlNode);
                pair = NodeContainer (switchNode);
                pairDevs = csmaHelper.Install (pair);
                switchPorts.Add (pairDevs.Get (0));
                // switchPorts.Add (pairDevs.Get (1));

                // of13Helper->InstallSwitch (switchNode, switchPorts[0]);
                // of13Helper->InstallSwitch (switchNode, switchPorts[1]);
                of13Helper->InstallSwitch (switchNode, switchPorts);
                // FIXME: after CreatingOFChannels
                of13Helper->CreateOpenFlowChannels ();
                of13Helper->EnableOpenFlowPcap ("openflow");
                of13Helper->EnableDatapathStats ("switch-stats");
                csmaHelper.EnablePcap (loggingPath + sn->getName() + ".switch.pcap", switchPorts, true);
            }else{
                cout << "!!! NO LINK FOUND FOR SWITCH !!!" << endl;

            }
        }
    }
    // FIXME: should be here... (but adapted)
    // csmaHelper.EnablePcap (loggingPath + sn->getName() + ".switch.pcap", switchPorts [0], true);


    cout << "  ==> Wiring remaining Links" << endl;
    for(auto &it : seedParser.getLinks())
    {
        auto sl = &it.second;


        uint32_t srcNodeID = 9999999;
        uint32_t dstNodeID = 9999999;
        for(auto &nodeIt : allSeedNodes)
        {
            auto sn = &nodeIt.second;
            // cout << "node has interfaces: " << sn->getInterfaces().size() << endl;
            for(auto &si : sn->getInterfaces())
            {
                if(si->getType() == "control")
                {
                    continue;
                }
                // auto si = &interfacesIt.second;
                // cout << "interfaceName: " << si->getName() << endl;
                if(si->getName() == sl->getSource() )
                {
                    // cout << "from: " << si->getName() << "@" << sn->getName() << endl;
                    srcNodeID = sn->getIndex();
                }

                if(si->getName() == sl->getDestination() )
                {
                    // cout << "to " << si->getName() << "@" << sn->getName() << endl;
                    dstNodeID = sn->getIndex();
                }
            }
        }

        if(srcNodeID != 9999999 && dstNodeID != 9999999)
        {
            cout << "    --> Wiring link \"" << sl->getName () << "\"" << endl;
            cout << "      --> from: \"" << sl->getSource() << "\"" << endl;
            cout << "      --> to: \"" << sl->getDestination() << "\"" << endl;

            PointToPointHelper pointToPoint;
            pointToPoint.SetDeviceAttribute ("DataRate", StringValue ( sl->getBandwidth() ));
            pointToPoint.SetChannelAttribute ("Delay", StringValue ( sl->getDelay() ));
            NodeContainer tmpNodeContainer;


            tmpNodeContainer.Add (allNodesContainer.Get (srcNodeID) );
            tmpNodeContainer.Add (allNodesContainer.Get (dstNodeID) );
            NetDeviceContainer tmpNetDeviceContainer = pointToPoint.Install (tmpNodeContainer);

            Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
            em->SetAttribute ("ErrorRate", DoubleValue (sl->getErrorRate()));
            // cout << "ErrorRate: " << sl->getErrorRate() << endl;
            tmpNetDeviceContainer.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

            ipv4.NewNetwork();
            Ipv4InterfaceContainer tmpIpAddresses = ipv4.Assign (tmpNetDeviceContainer);
            // cout << srcNodeID << " - " << dstNodeID << " - " << sl->getBandwidth() << " - " << sl->getDelay() << " - " << sl->getErrorRate() <<  endl;
            allIpAddresses.Add ( tmpIpAddresses );
            linkIndex++;
        }
    }



    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: Misc Setup" << endl;
    cout << "  --> Populating Routing Tables" << endl;
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();





    // EVENTS
    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: Scheduling Events" << endl;
    ApplicationContainer allApplications;
    ApplicationContainer sinkApps;
    uint32_t selectRandom = 1;
    for(auto const& eventPtr : seedParser.getEvents())
    {
        double startTime = eventPtr->getStartTime();
        shared_ptr<SeedBulkTrafficEvent> sbtePtr = dynamic_pointer_cast<SeedBulkTrafficEvent>(eventPtr);
        shared_ptr<SeedLinkChangeEvent> slcePtr = dynamic_pointer_cast<SeedLinkChangeEvent>(eventPtr);
        if(sbtePtr){
            uint32_t srcNodeID =  9999999;
            uint32_t dstNodeID =  9999999;

            std::string srcNameFin = sbtePtr->getSource();
            std::string dstNameFin = sbtePtr->getDestination();

            std::string srcName = sbtePtr->getSource();
            if(srcName.find("sample") != std::string::npos){
                unsigned first = srcName.find("(") + 1;
                unsigned last = srcName.find(")");
                std::string strNew = srcName.substr (first,last-first);
                // cout << "Select source node from: " << strNew << endl;

                vector<uint32_t> matchingNodes;
                for(auto &nodeIt : allSeedNodes)
                {
                    auto sn = &nodeIt.second;
                    if(sn->getGroups().find(strNew) != std::string::npos){
                        matchingNodes.push_back(sn->getIndex());
                    }
                }
                // cout << matchingNodes.size() << " nodes in group " << strNew << endl;
                Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
                rand->SetAttribute( "Min", DoubleValue( 0 ) );
                rand->SetAttribute( "Max", DoubleValue( matchingNodes.size()-1 ) );
                rand->SetAttribute( "Stream", IntegerValue( selectRandom * 42) );
                srcNodeID = matchingNodes[rand->GetInteger()];
                // cout << "selected srcNodeID " << srcNodeID << endl;
                selectRandom++;
            }

            std::string dstName = sbtePtr->getDestination();
            if(dstName.find("sample") != std::string::npos){
                unsigned first = dstName.find("(") + 1;
                unsigned last = dstName.find(")");
                std::string strNew = dstName.substr (first,last-first);
                // cout << "Select destination node from: " << strNew << endl;

                vector<uint32_t> matchingNodes;
                for(auto &nodeIt : allSeedNodes)
                {
                    auto sn = &nodeIt.second;
                    if(sn->getGroups().find(strNew) != std::string::npos){
                        matchingNodes.push_back(sn->getIndex());
                    }
                }
                // cout << matchingNodes.size() << " nodes in group " << strNew << endl;
                Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
                rand->SetAttribute( "Min", DoubleValue( 0 ) );
                rand->SetAttribute( "Max", DoubleValue( matchingNodes.size()-1 ) );
                rand->SetAttribute( "Stream", IntegerValue( selectRandom * 23 ) );
                dstNodeID = matchingNodes[rand->GetInteger()];
                // cout << "selected dstNodeID " << dstNodeID << endl;

                selectRandom++;
            }

            for(auto &nodeIt : allSeedNodes)
            {
                auto sn = &nodeIt.second;
                // cout << "name: " << sn->getName() << endl;
                // cout << "index: " << sn->getIndex() << endl;
                if(sn->getName() == srcNameFin )
                {
                    // cout << "src: " << sbtePtr->getSource() << endl;
                    srcNodeID = sn->getIndex();
                }
                if(sn->getName() == dstNameFin )
                {
                    dstNodeID = sn->getIndex();
                    // cout << "dst: " << sbtePtr->getDestination() << endl;
                }
            }


            // cout << " " << endl;

            Ptr <Node> PtrNodeSrc = allNodesContainer.Get ( srcNodeID );
            Ptr <Node> PtrNodeDst = allNodesContainer.Get ( dstNodeID );
            // Ptr<Ipv4> ipv4src = PtrNodeSrc->GetObject<Ipv4> ();
            Ptr<Ipv4> ipv4dst = PtrNodeDst->GetObject<Ipv4> ();
            // Ipv4InterfaceAddress iaddrSrc = ipv4src->GetAddress (1,0);
            Ipv4InterfaceAddress iaddrDst = ipv4dst->GetAddress (1,0);
            // Ipv4Address ipAddrSrc = iaddrSrc.GetLocal ();
            Ipv4Address ipAddrDst = iaddrDst.GetLocal ();

            // cout << ipAddrSrc << endl;
            // cout << ipAddrDst << endl;
            uint16_t port = 1337 + dstNodeID;  // well-known echo port number

            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                    InetSocketAddress (Ipv4Address::GetAny (), port));
            sinkApps = sink.Install (allNodesContainer.Get (dstNodeID));
            sinkApps.Start (Seconds ( (startTime - 0.1) ));
            sinkApps.Stop (Seconds ( (runTime) ));

            BulkSendHelper source ("ns3::TcpSocketFactory",
                    InetSocketAddress (ipAddrDst, port));
            source.SetAttribute ("MaxBytes", UintegerValue ( sbtePtr->getSize() ));
            // cout << "transfer: " << sbtePtr->getSize() << endl;
            ApplicationContainer sourceApps = source.Install (allNodesContainer.Get (srcNodeID));
            sourceApps.Start (Seconds (startTime));
            sourceApps.Stop (Seconds (runTime));


            allApplications.Add (sinkApps);
            allApplications.Add (sourceApps);

            cout << "  => BulkSend scheduled for " << startTime << "s" << endl;
            cout << "     --> BulkTrafficEvent from " << srcNodeID << " to " << dstNodeID << "" << endl;
        }else if(slcePtr){
            for(auto &it : seedParser.getLinks())
            {
                auto sl = &it.second;
                if( sl->getName() == slcePtr->getLink() ){

                    uint32_t srcNodeID = 9999999;
                    uint32_t dstNodeID = 9999999;
                    for(auto &nodeIt : allSeedNodes)
                    {
                        auto sn = &nodeIt.second;
                        for(auto &si : sn->getInterfaces())
                        {
                            if(si->getName() == sl->getSource() )
                            {
                                srcNodeID = sn->getIndex();
                            }

                            if(si->getName() == sl->getDestination() )
                            {
                                dstNodeID = sn->getIndex();
                            }
                        }
                    }

                    Ptr <Node> ptrNode;
                    if(srcNodeID != 9999999)
                    {
                        ptrNode = allNodesContainer.Get ( srcNodeID );
                    }
                    else if(dstNodeID != 9999999)
                    {
                        ptrNode = allNodesContainer.Get ( dstNodeID );
                    }
                    Ptr<Ipv4> tmpIpv4 = ptrNode->GetObject<Ipv4> ();
                    uint32_t ipv4ifIndex = 1;

                    if( slcePtr->getState() ){
                        Simulator::Schedule (Seconds (startTime), &Ipv4::SetUp, tmpIpv4, ipv4ifIndex);
                        cout << "  => LinkChange \"UP\" scheduled for " << startTime << "s" << endl;
                    }else{
                        Simulator::Schedule (Seconds (startTime), &Ipv4::SetDown, tmpIpv4, ipv4ifIndex);
                        cout << "  => LinkChange \"DOWN\" scheduled for " << startTime << "s" << endl;

                    }
                    break;
                }
            }
        }else{
            cout << "  Error: Whoops something went wrong - should not have done this..." << endl;
        }
    }



    // fallback pos
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
    positionAlloc ->Add(Vector(1, 1, 1)); // A
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(allNodesContainer);



    // Set Position and desc for animation
    AnimationInterface anim (loggingPath + loggingName + ".animate");
    anim.SetMobilityPollInterval (Seconds (1));
    anim.EnablePacketMetadata (true);
    for(auto &it : allSeedNodes)
    {
        auto sn = &it.second;
        Ptr< Node > tmpNode = allNodesContainer.Get (sn->getIndex());

        // set X/Y-pos
        double posX = sn->getPosX();
        double posY = sn->getPosY();
        if(posX == 0){
            Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
            x->SetAttribute( "Min", DoubleValue( 1 ) );
            x->SetAttribute( "Max", DoubleValue( 1000 ) );
            x->SetAttribute( "Stream", IntegerValue( sn->getIndex() + 1 ) );
            posX = x->GetInteger();
        }
        if(posY == 0){
            Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
            y->SetAttribute( "Min", DoubleValue( 1 ) );
            y->SetAttribute( "Max", DoubleValue( 1000 ) );
            y->SetAttribute( "Stream", IntegerValue( sn->getIndex() * 10 ) );
            posY = y->GetInteger();
        }
        // cout << "using X: " << posX << " and Y: " << posY << endl;
        anim.SetConstantPosition (tmpNode, posX, posY, 1);

        switch(sn->getType())
        {
            case NodeType::HOST:
                anim.UpdateNodeDescription (tmpNode, "Host \"" + sn->getName() + "\"");
                break;
            case NodeType::GENERIC_SWITCH:
                anim.UpdateNodeDescription (tmpNode, "Switch \"" + sn->getName() + "\"");
                break;
            case NodeType::OF_SWITCH:
                anim.UpdateNodeDescription (tmpNode, "SDN-Switch \"" + sn->getName() + "\"");
                break;
            case NodeType::OF_CONTROLLER:
                anim.UpdateNodeDescription (tmpNode, "SDN-Controller \"" + sn->getName() + "\"");
                break;
            case NodeType::UNDEFINED:
            default:
                cout << "Error" << endl;
                break;
        }
    }
    // LOGGING
    // AsciiTraceHelper ascii;
    cout << endl;
    cout << "===============================================" << endl;
    cout << "==> SEED-NS3: Initializing Logging" << endl;

    // log initial globla routing
    Ipv4GlobalRoutingHelper globalRouting;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (loggingPath + loggingName + ".routes", std::ios::out);
    globalRouting.PrintRoutingTableAllAt (Seconds(0.1), routingStream );


    // trace events
    AsciiTraceHelper ascii;
    PointToPointHelper pointToPoint;
    pointToPoint.EnableAsciiAll (ascii.CreateFileStream (loggingPath + loggingName + ".tr"));

    // pcap logging
    if(pcapEnabled)
    {
        pointToPoint.EnablePcapAll (loggingPath + loggingName, false);
    }

    // Flow Monitor
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    flowmon = flowmonHelper.InstallAll ();




    // SETUP DONE - STARTING SIMULATION
    cout << endl;
    cout << "===============================================" << endl;
    cout << "=> SEED-NS3: SETUP DONE" << endl;
    cout << "==> SEED-NS3: Starting Simulation (for " << runTime << "s)" << endl;

    Packet::EnablePrinting ();
    NS_LOG_INFO ("Run Simulation");
    Simulator::ScheduleNow(&printCurrentSimulationTime, runTime);
    Simulator::Stop (Seconds (runTime));
    Simulator::Run ();
    NS_LOG_INFO ("Done.");
    flowmon->CheckForLostPackets ();
    flowmon->SerializeToXmlFile(loggingPath + loggingName + ".flowmon", true, true);
    Simulator::Destroy ();

    cout << "====> SEED-NS3: DONE" << endl;
    cout << endl;

    // TODO: post-run hooks?
    // Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
    // cout << "Total Bytes Received: " << sink1->GetTotalRx () << endl;
    // Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
    // std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
    // Ptr<PacketSink> sink2 = DynamicCast<PacketSink> (sinkApps.Get (1));
    // std::cout << "Total Bytes Received: " << sink2->GetTotalRx () << std::endl;

    return 0;
}
