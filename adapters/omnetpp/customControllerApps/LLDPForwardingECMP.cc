#include <LLDPForwardingECMP.h>
#include <algorithm>
#include <string>
#include <queue>

using namespace std;

struct comp {
    bool operator() (const pair<string,int> &a, const pair<string,int> &b) {
        return a.second > b.second;
    }
};

Define_Module(LLDPForwardingECMP);


LLDPForwardingECMP::LLDPForwardingECMP(){

}

LLDPForwardingECMP::~LLDPForwardingECMP(){

}

void LLDPForwardingECMP::initialize(){
    AbstractControllerApp::initialize();
    dropIfNoRouteFound = par("dropIfNoRouteFound");
    ignoreArpRequests = par("ignoreArpRequests");
    printMibGraph = par("printMibGraph");
    timeOut = par("timeOut");
    lldpAgent = NULL;
}

void LLDPForwardingECMP::handlePacketIn(OFP_Packet_In * packet_in_msg){
    //get some details
    CommonHeaderFields headerFields = extractCommonHeaderFields(packet_in_msg);

    //ignore lldp packets
    if(headerFields.eth_type == 0x88CC){
        return;
    }

    //ignore arp requests
    if(ignoreArpRequests && headerFields.eth_type == ETHERTYPE_ARP && packet_in_msg->getMatch().OFB_ARP_OP == ARP_REQUEST){
        return;
    }

    //compute path for non arps
    std::list<LLDPPathSegment> route = computePath(headerFields.swInfo->getMacAddress(),headerFields.dst_mac.str());

    //if route empty flood
    if(route.empty()){
        if (headerFields.eth_type == ETHERTYPE_ARP) {
          int connId = controller->findSwitchInfoFor(packet_in_msg)->getConnId();
          auto fCKey = std::make_tuple(connId, headerFields.arp_op, headerFields.arp_src_adr, headerFields.arp_dst_adr);
          auto fCRes = floodCache.insert(std::make_pair(fCKey, SIMTIME_ZERO));
          if (fCRes.second || fCRes.first->second + timeOut < simTime())
          {
              floodPacket(packet_in_msg);
          }
          fCRes.first->second = simTime();
        } else if(dropIfNoRouteFound){
            dropPacket(packet_in_msg);
        } else {
            floodPacket(packet_in_msg);
        }
    } else {
        std::string computedRoute = "";
        //send packet to next hop
        sendPacket(packet_in_msg,route.front().outport);

        //iterate the rest of the route and set flow mods for switches under my control
        for (auto &seg : route){
            //concatenate route
            computedRoute += seg.chassisId + " -> ";

            oxm_basic_match match = oxm_basic_match();
            match.OFB_ETH_SRC = headerFields.src_mac;
            match.OFB_ETH_DST = headerFields.dst_mac;
            match.OFB_ETH_TYPE = headerFields.eth_type;

            match.wildcards= 0;
            match.wildcards |= OFPFW_IN_PORT;

            TCPSocket * socket = controller->findSocketForChassisId(seg.chassisId);

            //is switch under our control
            if(socket != NULL){
                sendFlowModMessage(OFPFC_ADD, match, seg.outport, socket,par("flowModIdleTimeOut"),par("flowModHardTimeOut"));
            }
        }

        //clean up route
        computedRoute.erase(computedRoute.length()-4);
        EV << "Route:" << computedRoute << endl;
    }
}

void LLDPForwardingECMP::receiveSignal(cComponent *src, simsignal_t id, cObject *obj) {
    AbstractControllerApp::receiveSignal(src,id,obj);

    //set lldp link
    if(lldpAgent == NULL && controller != NULL){
        std::vector<AbstractControllerApp *>* appList = controller->getAppList();
        std::vector<AbstractControllerApp *>::iterator iterApp;

        for(iterApp=appList->begin();iterApp!=appList->end();++iterApp){
            if(dynamic_cast<LLDPAgent *>(*iterApp) != NULL) {
                LLDPAgent *lldp = (LLDPAgent *) *iterApp;
                lldpAgent = lldp;
                break;
            }
        }
    }

    if(id == PacketInSignalId){
        EV << "LLDPForwardingECMP::PacketIn" << endl;
        if (dynamic_cast<OFP_Packet_In *>(obj) != NULL) {
            OFP_Packet_In *packet_in_msg = (OFP_Packet_In *) obj;
            handlePacketIn(packet_in_msg);
        }
    }
}

std::list<LLDPPathSegment> LLDPForwardingECMP::computePath(std::string srcId, std::string dstId){
    LLDPMibGraph *mibGraph = lldpAgent->getMibGraph();
    mibGraph->removeExpiredEntries();
    std::map<std::string, std::vector<LLDPMib> > verticies = mibGraph->getVerticies();

    EV << "Finding Route in " << mibGraph->getNumOfVerticies() << " Verticies and " << mibGraph->getNumOfEdges() << " Edges" << endl;
    if(printMibGraph){
        EV << mibGraph->getStringGraph() << endl;
    }

    std::list<LLDPPathSegment> result = std::list<LLDPPathSegment>();

    //quick check if we have src and target in our graph
    if(verticies.count(srcId)<=0 || verticies.count(dstId)<=0){
        return result;
    }

    //dijkstra
    std::map<std::string,int> dist;
    std::map<std::string,std::vector<LLDPPathSegment>> prev;
    std::map<std::string,bool> visited;

    //extract all vertexes
    priority_queue< pair<string,int>, vector< pair<string,int> >, comp > q;

    std::map<std::string,std::vector<LLDPMib> >::iterator iterKey = verticies.begin();
    while (iterKey != verticies.end()) {
        if(strcmp(iterKey->first.c_str(),srcId.c_str())!=0){
            q.push(pair<string,int>(iterKey->first,std::numeric_limits<int>::max()));
            dist[iterKey->first] = std::numeric_limits<int>::max();
        } else {
            //set start vertex
            q.push(pair<string,int>(srcId,0));
            dist[iterKey->first] = 0;
        }
        prev[iterKey->first] = std::vector<LLDPPathSegment>();
        iterKey++;
    }

    std::string u;
    while(!q.empty()){

        u = q.top().first;
        q.pop();

        if(visited.count(u)>0){
            continue;
        }

        std::vector<LLDPMib>::iterator iterList;
        for(iterList = verticies[u].begin();iterList!=verticies[u].end();iterList++){
            int alt = dist[u]+1;
            LLDPPathSegment seg;
            seg.chassisId = u;
            seg.outport= (iterList)->getSrcPort();
            if(alt < dist[iterList->getDstId()]){
                dist[iterList->getDstId()] = alt;
                prev[(iterList)->getDstId()].clear();
                prev[(iterList)->getDstId()].push_back(seg);
                q.push(pair<string,int>(iterList->getDstId(),alt));
            } else if (alt == dist[iterList->getDstId()]){
                prev[(iterList)->getDstId()].push_back(seg);
            }
        }

        //add u to visited
        visited[u] = true;
    }

    //back track and insert into list
    std::string trg = dstId;
    while(!prev[trg].empty()){
        LLDPPathSegment segP = prev[trg][intuniform(0, prev[trg].size()-1)];
        result.push_front(segP);
        trg = segP.chassisId;
    }

    //check if there was route from src to dst
    if(strcmp(srcId.c_str(),trg.c_str()) != 0){
        result.clear();
    }
    std::list<LLDPPathSegment> res2 = std::list<LLDPPathSegment>();
    std::copy(result.begin(),result.end(), std::back_inserter(res2));
    return res2;
}
