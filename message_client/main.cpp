#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <string>
#include <cstdio> // [s]print[f]

#include "gloox/client.h"
#include "gloox/connectionlistener.h"
#include "gloox/disco.h"
#include "gloox/message.h"
#include "gloox/gloox.h"
#include "gloox/loghandler.h"
#include "gloox/tlshandler.h"
#include "gloox/tlsdefault.h"
#include "gloox/logsink.h"
#include "gloox/messagehandler.h"
#include "gloox/base64.h"
#include "gloox/tag.h"
#include "gloox/taghandler.h"

using namespace gloox;
using namespace std;

const std::string jidString = "test@192.168.1.105/client";

/*
 * Using TLS to encrypt end-to-end traffic is not a recommended practice in XMPP,
 * nor is it standardized in any way. Use this code at your own risk.
 */

class MessageTest : public ConnectionListener, LogHandler,
        MessageHandler, TLSHandler, TagHandler
{
public:
    MessageTest()
        : m_tls( new TLSDefault( this, "", TLSDefault::AnonymousClient ) ),
          rcpt( "gloox@192.168.1.105/server" ) {}

    virtual ~MessageTest()
    {
        delete m_tls;
    }

    void start()
    {
        JID jid(jidString);
        client = new Client( jid, "test" );
        client->registerConnectionListener( this );
        client->registerMessageHandler( this );
        client->registerTagHandler(this, "iq", "");
        client->logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this );
        client->connect();

        delete client;
    }

    virtual void onConnect()
    {
        printf( "connected!\n" );
        m_tls->handshake();
        xtlsSend();
        m_send = "";
    }

    virtual void onDisconnect( ConnectionError e )
    {
        printf( "message_test: disconnected: %d\n", e );
        if( e == ConnAuthenticationFailed )
            printf( "auth failed. reason: %d\n", client->authError() );
    }

    virtual bool onTLSConnect( const CertInfo& info )
    {
        time_t from( info.date_from );
        time_t to( info.date_to );

        printf( "status: %d\nissuer: %s\npeer: %s\nprotocol: %s\nmac: %s\ncipher: %s\ncompression: %s\n"
                "from: %s\nto: %s\n",
                info.status, info.issuer.c_str(), info.server.c_str(),
                info.protocol.c_str(), info.mac.c_str(), info.cipher.c_str(),
                info.compression.c_str(), ctime( &from ), ctime( &to ) );
        return true;
    }

    void xtlsSend()
    {
        Tag *m = new Tag( "iq", "xmlns", "xmpp:iq:gloox" );
        m->addAttribute( "to", rcpt.full() );
        m->addAttribute( "from", jidString );
        m->addAttribute( "id", "client" );
        Tag *x = new Tag( "dreq", "xmlns", "status" );
        m->addChild(x);
        client->send( m );
    }

    virtual void handleEncryptedData( const TLSBase* /*base*/, const std::string& data )
    {
        printf( "caching %d bytes of encrypted data\n", (int)data.length() );
        m_send += data;
    }

    virtual void handleDecryptedData( const TLSBase* /*base*/, const std::string& data )
    {
        printf( "decrypted packet contents: %s\n", data.c_str() );
        m_tls->encrypt( "bye" );
        xtlsSend();
        client->disconnect();
    }

    virtual void handleHandshakeResult( const TLSBase* /*base*/, bool success, CertInfo& /*certinfo*/ )
    {
        if( success )
        {
            printf( "xtls handshake successful!\n" );
            m_tls->encrypt( "ping" );
            xtlsSend();
        }
        else
        {
            printf( "xtls handshake failed!\n" );
            client->disconnect();
        }
    }

    virtual void handleTag(Tag *tag)
    {
        if(tag) {
            printf("tag name: %s, xml: %s\n", tag->name().c_str(), tag->xml().c_str());
            xtlsSend();
            sleep(2);
        }
    }

    virtual void handleMessage( const Message& msg, MessageSession * /*session*/ )
    {
        Tag* m = msg.tag();
        Tag *x = m->findChild( "xtls", "xmlns", "test:xtls" );
        if( x )
        {
            printf( "decrypting: %d\n", (int)x->cdata().length() );
            m_tls->decrypt( Base64::decode64( x->cdata() ) );
            xtlsSend();
        }
        delete m;
    }

    virtual void handleLog( LogLevel level, LogArea area, const std::string& message )
    {
        printf("log: level: %d, area: %d, %s\n", level, area, message.c_str() );
    }

private:
    Client *client;
    TLSBase* m_tls;
    std::string m_send;
    const JID rcpt;
};

int main( int /*argc*/, char** /*argv*/ )
{
    MessageTest *r = new MessageTest();
    r->start();
    delete( r );
    return 0;
}
