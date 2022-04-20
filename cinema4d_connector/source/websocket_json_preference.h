#ifndef WEBSOCKET_JSON_CE_PREF_H__
#define WEBSOCKET_JSON_CE_PREF_H__

#include "maxon/code_exchange.h"
#include "c4d.h"
#include "lib_prefs.h"
#include "c4d_basecontainer.h"

#define CODEEXCHANGE_PREFS	1058660 // Registered by Cinema 4D
#define WEBSOCKET_JSON_PREFS 1058664
#define DEFAULT_PORT_VALUE	7788

class WebSocketJsonCodeExchangePreferences : public PrefsDialogObject
{
	INSTANCEOF(WebSocketJsonCodeExchangePreferences, PrefsDialogObject)

public:
	virtual Bool InitValues(const DescID& id, Description* desc = nullptr);

	virtual Bool Init(GeListNode* node);
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags);
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags);
	virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags);

	static NodeData* Alloc() { return NewObjClear(WebSocketJsonCodeExchangePreferences); }
};

Bool RegisterWebSocketJsonCodeExchangePreferences();
BaseContainer* GetPreferences();
Int32 GetPortFromPreference();


#endif // WEBSOCKET_JSON_CE_PREF_H__