// Local Includes
#include "websocketservice.h"
#include "websocketserver.h"
#include "websocketservercomponent.h"

// External Includes
#include <nap/logger.h>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::WebSocketService)
	RTTI_CONSTRUCTOR(nap::ServiceConfiguration*)
RTTI_END_CLASS

namespace nap
{
	WebSocketService::WebSocketService(ServiceConfiguration* configuration) :
		Service(configuration)
	{
	}


    bool WebSocketService::init(nap::utility::ErrorState& errorState)
    {
		return true;
    }    


	void WebSocketService::registerObjectCreators(rtti::Factory& factory)
	{
		factory.addObjectCreator(std::make_unique<WebSocketServerObjectCreator>(*this));
	}


	void WebSocketService::update(double deltaTime)
	{
		std::queue<WebSocketEventPtr> events;
		for (auto& server : mServers)
		{
			server->consumeEvents(events);
			while (!(events.empty()))
			{
				WebSocketEvent* websocket_event = events.front().get();
				for (auto& component : mComponents)
				{
					if(&component->getServer() != server)	
						continue;

					WebSocketMessageReceivedEvent* msg_received = rtti_cast<WebSocketMessageReceivedEvent>(websocket_event);
					if (msg_received != nullptr)
					{
						component->messageReceived(*msg_received);
						continue;
					}

					WebSocketConnectionOpenedEvent* con_opened = rtti_cast<WebSocketConnectionOpenedEvent>(websocket_event);
					if (con_opened != nullptr)
					{
						component->connectionOpened(*con_opened);
						continue;
					}

					WebSocketConnectionClosedEvent* con_closed = rtti_cast<WebSocketConnectionClosedEvent>(websocket_event);
					if (con_closed != nullptr)
					{
						component->connectionClosed(*con_closed);
						continue;
					}

					WebSocketConnectionFailedEvent* con_failed = rtti_cast<WebSocketConnectionFailedEvent>(websocket_event);
					if (con_failed != nullptr)
					{
						component->connectionFailed(*con_failed);
						continue;
					}

					// Unknown message web-socket event type
					assert(false);
				}				
				events.pop();
			}
		}
	}


	void WebSocketService::registerServer(WebSocketServer& server)
	{
		mServers.emplace_back(&server);
	}


	void WebSocketService::removeServer(WebSocketServer& server)
	{
		auto found_it = std::find_if(mServers.begin(), mServers.end(), [&](const auto& it)
		{
			return it == &server;
		});
		
		assert(found_it != mServers.end());
		mServers.erase(found_it);
	}


	void WebSocketService::registerComponent(WebSocketServerComponentInstance& component)
	{
		mComponents.emplace_back(&component);
	}


	void WebSocketService::removeComponent(WebSocketServerComponentInstance& component)
	{
		auto found_it = std::find_if(mComponents.begin(), mComponents.end(), [&](const auto& it)
		{
			return it == &component;
		});

		assert(found_it != mComponents.end());
		mComponents.erase(found_it);
	}

}
