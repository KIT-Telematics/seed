#include "context.h"
#include <string.h>
#include <cstdio>
#include <omnetpp.h>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"
#include <expat.h>

#include "parser.h"

using namespace rapidjson;

class SeedSimple : public cSimpleModule
{
  private:
    std::unique_ptr<cMessage> event;
    Context context;

  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
// The module class needs to be registered with OMNeT++
Define_Module(SeedSimple);
void SeedSimple::initialize()
{
    event = std::unique_ptr<cMessage>(new cMessage("initialize"));

    scheduleAt(simTime(), event.get());
}

void SeedSimple::handleMessage(cMessage *msg)
{
    if (msg == event.get())
    {
        if (Parser::parse(par("bundle_path"), context, this))
        {
            std::cout << "Bundle loaded successfully!" << std::endl;
        }
        else
        {
            std::cout << "Failed to load Bundle!" << std::endl;
            exit(1);
        }

        {
            Document d;
            d.SetObject();
            Document::AllocatorType& a = d.GetAllocator();

            Value nodes(kObjectType);
            for (auto it = context.n.begin(); it != context.n.end();  ++it) {
                Value k(it->second->getFullPath().c_str(), a);
                Value v(it->first.c_str(), a);
                nodes.AddMember(k, v, a);
            }
            d.AddMember("nodes", nodes, a);

            Value interfaces(kObjectType);
            for (auto it = context.i.begin(); it != context.i.end();  ++it) {
                auto l = it->second;
                //TODO: Gate mapping not useful this way
                Value k1(l.first->getFullPath().c_str(), a);
                Value k2(l.second->getFullPath().c_str(), a);
                Value v(it->first.c_str(), a);
                interfaces.AddMember(k1, v, a);
                interfaces.AddMember(k2, v, a);
            }
            d.AddMember("interfaces", interfaces, a);

            Value links(kObjectType);
            for (auto it = context.l.begin(); it != context.l.end();  ++it) {
                auto l = it->second;
                Value k1(l.first->getFullPath().c_str(), a);
                Value v1((it->first + "::a").c_str(), a);
                links.AddMember(k1, v1, a);
                Value k2(l.second->getFullPath().c_str(), a);
                Value v2((it->first + "::b").c_str(), a);
                links.AddMember(k2, v2, a);
            }
            d.AddMember("links", links, a);

            FILE* fp = fopen("out/mapping.json", "w");
            char writeBuffer[65536];
            FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
            PrettyWriter<FileWriteStream> w(os);
            d.Accept(w);
        }

        context.factory->instantiate(this);
    }
}
