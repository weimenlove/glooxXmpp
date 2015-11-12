#include <unistd.h>
#include <stdio.h>
#include <string>
#include <cstdio> // [s]print[f]

#include "gloox/client.h"
#include "gloox/messagesessionhandler.h"
#include "gloox/messageeventhandler.h"
#include "gloox/messageeventfilter.h"
#include "gloox/chatstatehandler.h"
#include "gloox/chatstatefilter.h"
#include "gloox/connectionlistener.h"
#include "gloox/disco.h"
#include "gloox/message.h"
#include "gloox/gloox.h"
#include "gloox/lastactivity.h"
#include "gloox/loghandler.h"
#include "gloox/logsink.h"
#include "gloox/connectiontcpclient.h"
#include "gloox/connectionsocks5proxy.h"
#include "gloox/connectionhttpproxy.h"
#include "gloox/messagehandler.h"

using namespace gloox;
using namespace std;

const std::string jidString = "gloox@192.168.1.105/gloox";

class MessageTest : public MessageSessionHandler, ConnectionListener, LogHandler,
        MessageEventHandler, MessageHandler, ChatStateHandler
{
public:
    MessageTest() : m_session(0), m_messageEventFilter(0), m_chatStateFilter(0)
    {}

    virtual ~MessageTest() {}

    void start()
    {
        JID jid(jidString);
        client = new Client(jid, "gloox");
        client->registerConnectionListener(this);
        client->registerMessageSessionHandler(this, 0);
        client->disco()->setVersion("messageTest", GLOOX_VERSION, "Linux");
        client->disco()->setIdentity("client", "bot");
        client->disco()->addFeature(XMLNS_CHAT_STATES);
        StringList ca;
        ca.push_back( "/path/to/cacert.crt");
        client->setCACerts(ca);

        // debug info level
//        client->logInstance().registerLogHandler(LogLevelDebug, LogAreaAll, this);
        client->logInstance().registerLogHandler(LogLevelError, LogAreaAll, this);

        //
        // this code connects to a jabber server through a SOCKS5 proxy
        //
        //       ConnectionSOCKS5Proxy* conn = new ConnectionSOCKS5Proxy(client,
        //                                   new ConnectionTCP(client->logInstance(),
        //                                                      "sockshost", 1080),
        //                                   client->logInstance(), "example.net");
        //       conn->setProxyAuth("socksuser", "sockspwd");
        //       client->setConnectionImpl(conn);

        //
        // this code connects to a jabber server through a HTTP proxy through a SOCKS5 proxy
        //
        //       ConnectionTCP* conn0 = new ConnectionTCP(client->logInstance(), "old", 1080);
        //       ConnectionSOCKS5Proxy* conn1 = new ConnectionSOCKS5Proxy(conn0, client->logInstance(), "old", 8080);
        //       conn1->setProxyAuth("socksuser", "sockspwd");
        //       ConnectionHTTPProxy* conn2 = new ConnectionHTTPProxy(client, conn1, client->logInstance(), "jabber.cc");
        //       conn2->setProxyAuth("httpuser", "httppwd");
        //       client->setConnectionImpl(conn2);

        if(client->connect(false))
        {
            ConnectionError ce = ConnNoError;
            while(ce == ConnNoError)
            {
                ce = client->recv();
            }
            printf("ce: %d\n", ce);
        }

        delete(client);
    }

    virtual void onConnect()
    {
        printf("connected!!!\n");
    }

    virtual void onDisconnect(ConnectionError e)
    {
        printf("message_test: disconnected: %d\n", e);
        if(e == ConnAuthenticationFailed)
            printf("auth failed. reason: %d\n", client->authError());
    }

    virtual bool onTLSConnect(const CertInfo& info)
    {
        time_t from(info.date_from);
        time_t to(info.date_to);

        printf("status: %d\nissuer: %s\npeer: %s\nprotocol: %s\nmac: %s\ncipher: %s\ncompression: %s\n"
                "from: %s\nto: %s\n",
                info.status, info.issuer.c_str(), info.server.c_str(),
                info.protocol.c_str(), info.mac.c_str(), info.cipher.c_str(),
                info.compression.c_str(), ctime( &from ), ctime( &to ));
        return true;
    }

    virtual void handleMessage(const Message& msg, MessageSession * /*session*/)
    {
        printf( "type: %d, subject: %s, message: %s, thread id: %s\n", msg.subtype(),
                msg.subject().c_str(), msg.body().c_str(), msg.thread().c_str());

        if(!msg.body().empty()) {
            std::string re = "You said: " + msg.body() + "\n";
            std::string sub;
            if(!msg.subject().empty())
                sub = "Re: " + msg.subject();

            m_messageEventFilter->raiseMessageEvent(MessageEventDisplayed);
            sleep(1);
            m_messageEventFilter->raiseMessageEvent(MessageEventComposing);
            m_chatStateFilter->setChatState(ChatStateComposing);
            sleep(2);

            m_session->send(re, sub);

            if(msg.body() == "quit")
                client->disconnect();
        }
    }

    virtual void handleMessageEvent(const JID& from, MessageEventType event)
    {
        printf("received event: %d from: %s\n", event, from.full().c_str());
    }

    virtual void handleChatState(const JID& from, ChatStateType state)
    {
        printf("received state: %d from: %s\n", state, from.full().c_str());
    }

    virtual void handleMessageSession(MessageSession *session)
    {
        printf("got new session\n");
        // this example can handle only one session. so we get rid of the old session
        client->disposeMessageSession(m_session);
        m_session = session;
        m_session->registerMessageHandler(this);
        m_messageEventFilter = new MessageEventFilter(m_session);
        m_messageEventFilter->registerMessageEventHandler(this);
        m_chatStateFilter = new ChatStateFilter(m_session);
        m_chatStateFilter->registerChatStateHandler(this);
    }

    virtual void handleLog(LogLevel level, LogArea area, const std::string& message)
    {
        printf("log: level: %d, area: %d, %s\n", level, area, message.c_str());
    }

private:
    Client *client;
    MessageSession *m_session;
    MessageEventFilter *m_messageEventFilter;
    ChatStateFilter *m_chatStateFilter;
};

int main(int /*argc*/, char** /*argv*/)
{
    MessageTest *r = new MessageTest();
    r->start();
    delete(r);
    return 0;
}
