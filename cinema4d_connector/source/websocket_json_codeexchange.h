#ifndef WBSOCK_JSON_CE_H__
#define WBSOCK_JSON_CE_H__

#include "maxon/interface.h"
#include "maxon/objectbase.h"
#include "maxon/network_websocket.h"
#include "maxon/fid.h"
#include "maxon/basearray.h"
#include "maxon/weakref.h"
#include "maxon/code_exchange.h"

namespace maxon
{

// Key in the JSON communication with VsCode Extension
namespace CODEEXCHANGE
{
	static const LiteralId ACTION				{ "action" };
	static const LiteralId VALUE				{ "value" };
	static const LiteralId DEBUG				{ "debug" };
	static const LiteralId SCRIPT_PATH	{ "script_path" };

	// ACTION ID for message received from the IDE
	namespace IDE2C4D
	{
		static const LiteralId GET_SCRIPT_CONTENT				{ "idea2c4d.get_script_content" };
		static const LiteralId SET_SCRIPT_CONTENT				{ "idea2c4d.set_script_content" };
		static const LiteralId LOAD_IN_SCRIPT_MANAGER		{ "idea2c4d.load_in_script_manager" };
		static const LiteralId EXECUTE_SCRIPT						{ "idea2c4d.execute" };
		static const LiteralId GET_PID									{ "idea2c4d.get_pid" };
		static const LiteralId GET_PATH									{ "idea2c4d.get_path" };
	}

	// ACTION ID for message send from Cinema 4D
	namespace C4D2IDE
	{
		static const LiteralId SET_SCRIPT_CONTENT		{ "c4d2ide.set_script_content" };
		static const LiteralId GET_PID							{ "c4d2ide.get_pid" };
		static const LiteralId GET_PATH							{ "c4d2ide.get_path" };
		static const LiteralId CONSOLE							{ "c4d2ide.console" };
	}
}

//----------------------------------------------------------------------------------------
/// WebSocket and JSON Python code communication for Cinema 4D and IDE(s).
///
/// Only 1 implementation of this interface is allowed and it should be singleton.
//----------------------------------------------------------------------------------------
class WebSocketJsonCodeExchangeInterface : MAXON_INTERFACE_BASES(CodeExchangeInterface)
{
	MAXON_INTERFACE(WebSocketJsonCodeExchangeInterface, MAXON_REFERENCE_NORMAL, "net.maxonsdk.interfaces.codeexchange.websocket_json");

public:

	//----------------------------------------------------------------------------------------
	/// Retrieves a list of ongoing connections.
	///
	/// @return	Connection list with Cinema 4D and IDE.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD BaseArray<WeakRef<const NetworkWebSocketConnectionRef>>* GetWebSockets();

	//----------------------------------------------------------------------------------------
	/// Returns the current WebSocket server running state.
	///
	/// @return	true if the WebSocket server is running otherwise, false.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD Bool IsRunning();
};


#include "websocket_json_codeexchange1.hxx"

MAXON_DECLARATION(maxon::Class<WebSocketJsonCodeExchangeRef>, WebSocketJsonCodeExchange, "net.maxonsdk.class.codeexchange.websocket_json");

#include "websocket_json_codeexchange2.hxx"

}

#endif // WBSOCK_JSON_CE_H__