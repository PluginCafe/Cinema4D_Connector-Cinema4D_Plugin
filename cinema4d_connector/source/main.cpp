#include "c4d_plugin.h"
#include "c4d_resource.h"
#include "websocket_json_preference.h"
#include "maxon/code_exchange.h"

::Bool PluginStart()
{
	if (!RegisterWebSocketJsonCodeExchangePreferences())
		return false;

	return true;
}

void PluginEnd()
{

}

::Bool PluginMessage(::Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
		{
			if (g_resource.Init() == false)
				return false;
			return true;
			break;
		}
	}

	return false;
}
