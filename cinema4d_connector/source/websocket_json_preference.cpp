#include "websocket_json_preference.h"
#include "websocket_json_codeexchange.h"
#include "pref_websocket_json_ce.h"
#include "c4d_symbols.h"
#include "maxon/code_exchange.h"

//----------------------------------------------------------------------------------------
/// Retrieves or creates the WebSocket BaseContainer preference from the world container
//----------------------------------------------------------------------------------------
BaseContainer* GetPreferences()
{
	BaseContainer* bc = GetWorldContainerInstance()->GetContainerInstance(WEBSOCKET_JSON_PREFS);
	if (!bc)
	{
		GetWorldContainerInstance()->SetContainer(WEBSOCKET_JSON_PREFS, BaseContainer());

		bc = GetWorldContainerInstance()->GetContainerInstance(WEBSOCKET_JSON_PREFS);
		if (!bc)
			return nullptr;
	}

	return bc;
}

//----------------------------------------------------------------------------------------
/// Retrieve the value of the port from the preference
//----------------------------------------------------------------------------------------
Int32 GetPortFromPreference()
{
	BaseContainer* bc = GetPreferences();
	if (bc == nullptr)
		return DEFAULT_PORT_VALUE;

	return bc->GetInt32(PREFS_WEBSOCKET_JSON_CE_PORT, DEFAULT_PORT_VALUE);
}

Bool WebSocketJsonCodeExchangePreferences::InitValues(const DescID& id, Description* desc)
{
	BaseContainer* bc = GetPreferences();
	if (!bc)
		return false;

	switch (id[0].id)
	{
		case PREFS_WEBSOCKET_JSON_CE_PORT:
			InitPrefsValue(PREFS_WEBSOCKET_JSON_CE_PORT, GeData(DEFAULT_PORT_VALUE), desc, id, bc);
			break;
	}

	return true;
}

Bool WebSocketJsonCodeExchangePreferences::Init(GeListNode* node)
{
	// Leave if no CodeExchangeImpl is present
	if (!InitValues(PREFS_WEBSOCKET_JSON_CE_PORT))
		return false;

	return true;
}

Bool WebSocketJsonCodeExchangePreferences::GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags)
{
	iferr_scope_handler
	{
		ApplicationOutput(err.GetMessage());
		return false;
	};

	if (!description || !description->LoadDescription("pref_websocket_json_ce"_s))
		return false;


	if (flags & DESCFLAGS_DESC::NEEDDEFAULTVALUE)
	{
		if (!InitValues(PREFS_WEBSOCKET_JSON_CE_PORT, description))
			return false;
	}

		flags |= DESCFLAGS_DESC::LOADED;
	return SUPER::GetDDescription(node, description, flags);
}

Bool WebSocketJsonCodeExchangePreferences::SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
{
	BaseContainer* bc = GetPreferences();
	if (!bc)
		return SUPER::SetDParameter(node, id, t_data, flags);

	// PREFS_USED_CODE_EXCHANGE Cycle value store the String value in the BaseContainer
	// Name of the CodeExchangeImpl is safer than index since the cycle is built from the registry CodeExchanges.
	if (id[0].id == PREFS_WEBSOCKET_JSON_CE_PORT)
	{
		Int32 ceUsed = t_data.GetInt32();
		bc->SetInt32(PREFS_WEBSOCKET_JSON_CE_PORT, ceUsed);
		flags |= DESCFLAGS_SET::PARAM_SET;

		// Restart the WebSocket server if the port changed
		MAXON_SCOPE
		{
			iferr_scope_handler
			{ return true; };

			maxon::WebSocketJsonCodeExchangeRef ce = maxon::WebSocketJsonCodeExchange().Create() iferr_return;
			Bool isRunning = ce.IsRunning();

			if (isRunning)
			{
				ce.Stop() iferr_return;
				ce.Start() iferr_return;
			}
		}

		return true;
	}

	return SUPER::SetDParameter(node, id, t_data, flags);
}

Bool WebSocketJsonCodeExchangePreferences::GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags)
{
	BaseContainer* bc = GetPreferences();
	if (!bc)
		return SUPER::GetDParameter(node, id, t_data, flags);

	if (id[0].id == PREFS_WEBSOCKET_JSON_CE_PORT)
	{
		t_data = bc->GetInt32(PREFS_WEBSOCKET_JSON_CE_PORT, DEFAULT_PORT_VALUE);
		flags |= DESCFLAGS_GET::PARAM_GET;
		return true;
	}

	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool RegisterWebSocketJsonCodeExchangePreferences()
{
	const auto* websock = maxon::CodeExchanges::Find(maxon::Id("net.maxonsdk.class.codeexchange.websocket_json"));
	if (websock == nullptr)
		return true;

	if (!PrefsDialogObject::Register(WEBSOCKET_JSON_PREFS, WebSocketJsonCodeExchangePreferences::Alloc, GeLoadString(IDS_WEBSOCKET_JSON_CE), "pref_websocket_json_ce"_s, CODEEXCHANGE_PREFS, 0))
		return false;

	return true;
}