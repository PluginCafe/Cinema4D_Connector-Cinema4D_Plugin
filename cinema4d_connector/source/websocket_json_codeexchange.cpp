#include "maxon/interface.h"
#include "maxon/hierarchyobject.h"
#include "maxon/parser.h"
#include "maxon/datadictionary.h"
#include "maxon/registrybase.h"
#include "maxon/application.h"
#include "maxon/valuereceiver.h"
#include "maxon/errortypes.h"
#include "maxon/system_process.h"
#include "maxon/basearray.h"
#include "maxon/spinlock.h"
#include "maxon/timer.h"

#include "maxon/code_exchange.h"
#include "maxon/network_websocket.h"
#include "maxon/py_element.h"
#include "maxon/py_element_manager.h"

#include "websocket_json_codeexchange.h"
#include "websocket_json_preference.h"
#include "c4d_baseobject.h"
#include "c4d_general.h"

namespace maxon
{

NetworkWebSocketServerRef g_wsServer = NetworkWebSocketServerRef::NullValue();
// console output is added to this string buffer. This String buffer is flush and sent ##consoleOutputInterval second.
String			g_consolOutputBuffer;
Spinlock		g_consolOutputBufferLock;

// Time in second of the timer responsible to send the content of g_consolOutputBuffer to all connected IDEs.
const Float32 consoleOutputInterval = 1;

/// ----- Utilities functions ------ 

static Result<DataDictionary> JsonStringToDataDict(const String& str)
{
	iferr_scope;

	if (str.IsEmpty())
		return IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed string is empty."_s);

	const ParserRef jsonParser = ParserClasses::JsonParser().Create() iferr_return;
	maxon::SingleValueReceiver<const maxon::DataDictionary&> readData;
	// In case of miss-formated string we just return an empty dict
	iferr (jsonParser.ReadString(str, PARSERFLAGS::NONE, GetUtf8DefaultDecoder(), readData))
	{
		return {};
	}

	return readData.Get().GetValue();
}

static Result<String> DataDictToJsonString(const DataDictionary& dict)
{
	iferr_scope;

	if (dict.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed dict is empty."_s);

	const maxon::ParserRef jsonParser = maxon::ParserClasses::JsonParser().Create() iferr_return;

	String out;
	jsonParser.Write(dict, out, false) iferr_return;
	return out;
}

static Id GetActionId(const DataDictionary& incomingMsg)
{
	iferr_scope_handler
	{
		return Id();
	};

	String actionStr = incomingMsg.Get<String>(CODEEXCHANGE::ACTION.ToString()) iferr_return;
	Id action;
	action.Init(actionStr) iferr_return;

	return action;
}

/// ----- OnMessage functions ------ 

static Result<void> GetScriptContent(const DataDictionary& inData, DataDictionary& outDict)
{
	iferr_scope;

	outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::SET_SCRIPT_CONTENT) iferr_return;
	String scriptPath = inData.Get<String>(CODEEXCHANGE::SCRIPT_PATH.ToString()) iferr_return;

	PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
	Opt<PythonElementBaseRef> pyOptElem = pyManager.FindPyElementByPath(scriptPath) iferr_return;

	String out;
	if (pyOptElem.HasValue())
	{
		PythonElementBaseRef pyElem = pyOptElem.GetValue() iferr_return;
		out = pyElem.GetScript() iferr_return;
	}

	if (out.IsEmpty())
		return OK;

	outDict.Set(CODEEXCHANGE::VALUE, out) iferr_return;
	outDict.Set(CODEEXCHANGE::SCRIPT_PATH.ToString(), scriptPath) iferr_return;

	return OK;
}

static Result<void> LoadInScriptManager(const DataDictionary& inData, const NetworkWebSocketConnectionRef& webSocket)
{
	iferr_scope;

	String scriptPath = inData.Get<String>(CODEEXCHANGE::SCRIPT_PATH.ToString()) iferr_return;
	String scriptContent = inData.Get<String>(CODEEXCHANGE::VALUE.ToString()) iferr_return;

	PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
	Opt<PythonElementBaseRef> pyOptElem = pyManager.FindPyElementByPath(scriptPath) iferr_return;

	if (pyOptElem.HasValue())
	{
		PythonElementBaseRef pyElem = pyOptElem.GetValue() iferr_return;
		const PythonElementScriptRef& castedPyElem = Cast<PythonElementScriptRef>(pyElem);
		if (castedPyElem)
		{
			castedPyElem.SetScript(scriptContent) iferr_return;
			castedPyElem.ShowInScriptManager() iferr_return;
			return OK;
		}
	}
	if (scriptPath.StartsWith("c4dfs"_s))
	{

		ExecuteOnMainThread([scriptPath, scriptContent]()
		{
			::String name = Url(scriptPath).GetName();
			BaseList2D* scriptOp = CreateNewPythonScript(name, scriptContent);
			if (MAXON_UNLIKELY(scriptOp == nullptr))
				return false;

			Int32 scriptId = GetDynamicScriptID(scriptOp);
			if (MAXON_UNLIKELY(scriptId == NOTOK))
				return false;

			SetActiveScriptObject(scriptId);

			return true;
		});
		return OK;
	}
	else if (scriptPath.StartsWith("file"_s))
	{
		ExecuteOnMainThread([scriptPath]()
		{
			const Filename file = MaxonConvert(scriptPath);
			BaseList2D* scriptOp = LoadPythonScript(file);
			if (MAXON_UNLIKELY(scriptOp == nullptr))
				return false;

			return true;
		});
		return OK;
	}
	else if (scriptPath.StartsWith("untitled"_s))
	{
		ExecuteOnMainThread([scriptContent, webSocket]()
		{
			iferr_scope_handler
			{
				return false;
			};

			::String name = "";
			BaseList2D * scriptOp = CreateNewPythonScript(name, scriptContent);
			if (MAXON_UNLIKELY(scriptOp == nullptr))
				return false;

			PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
			Opt<PythonElementBaseRef> newPyOptElem = pyManager.FindPyElementByBaseList2D(scriptOp) iferr_return;
			PythonElementBaseRef pyElem = newPyOptElem.GetValue() iferr_return;

			DataDictionary outDict;
			outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::SET_SCRIPT_CONTENT) iferr_return;
			outDict.Set(CODEEXCHANGE::VALUE, scriptContent) iferr_return;
			outDict.Set(CODEEXCHANGE::SCRIPT_PATH.ToString(), pyElem.GetPath()) iferr_return;
			String out = DataDictToJsonString(outDict) iferr_return;
			webSocket.Send(out) iferr_return;
			return true;
		});
	}
	
	return OK;
}

static Result<void> SetScriptContent(const DataDictionary& inData)
{
	iferr_scope;

	// Script Path not defined, so create a new script 
	String scriptPath = inData.Get<String>(CODEEXCHANGE::SCRIPT_PATH.ToString(), ""_s);
	String scriptContent = inData.Get<String>(CODEEXCHANGE::VALUE.ToString()) iferr_return;

	PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
	Opt<PythonElementBaseRef> pyOptElem = pyManager.FindPyElementByPath(scriptPath) iferr_return;

	if (pyOptElem.HasValue())
	{
		PythonElementBaseRef pyElem = pyOptElem.GetValue() iferr_return;
		pyElem.SetScript(scriptContent) iferr_return;
	}

	return OK;
}

static Result<void> ExecuteScript(const DataDictionary& inData)
{
	iferr_scope;

	String scriptPath = inData.Get<String>(CODEEXCHANGE::SCRIPT_PATH.ToString()) iferr_return;
	Bool debug = inData.Get<Bool>(CODEEXCHANGE::DEBUG.ToString(), false);
	String scriptContent = inData.Get<String>(CODEEXCHANGE::VALUE.ToString()) iferr_return;

	PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
	Opt<PythonElementBaseRef> pyOptElem = pyManager.FindPyElementByPath(scriptPath) iferr_return;

	if (pyOptElem.HasValue())
	{
		PythonElementBaseRef pyElem = pyOptElem.GetValue() iferr_return;
		const PythonElementScriptRef& castedPyElem = Cast<PythonElementScriptRef>(pyElem);
		if (castedPyElem)
		{
			castedPyElem.SetScript(scriptContent) iferr_return;
			castedPyElem.Execute(debug) iferr_return;
		}
	}
	else if (debug)
	{
		// Debug === only file, therefor if we can't find it means we need to import it first
		ExecuteOnMainThread([scriptPath]()
		{
			iferr_scope_handler
			{
				return false;
			};

			BaseList2D * scriptOp = LoadPythonScript(MaxonConvert(scriptPath));
			if (MAXON_UNLIKELY(scriptOp == nullptr))
				return false;

			PyElementManagerRef pyManager = PyElementManager().Create() iferr_return;
			Opt<PythonElementBaseRef> newPyOptElem = pyManager.FindPyElementByBaseList2D(scriptOp) iferr_return;
			PythonElementBaseRef pyElem = newPyOptElem.GetValue() iferr_return;

			pyElem.Execute(true) iferr_return;

			return true;
		});

	}
	else
	{
		// Creates a temporary Python script and execute it
		BaseList2D* scriptOp = static_cast<BaseList2D*>(AllocListNode(ID_PYTHONSCRIPT));
		if (MAXON_UNLIKELY(scriptOp == nullptr))
			return NullptrError(MAXON_SOURCE_LOCATION);

		if (MAXON_UNLIKELY(GetScriptHead(0) == nullptr))
			return NullptrError(MAXON_SOURCE_LOCATION);

		scriptOp->InsertUnderLast(GetScriptHead(0));

		maxon::PythonElementScriptRef newItem = maxon::PyScriptElementFactory().Create(scriptOp) iferr_return;
		if (MAXON_UNLIKELY(!newItem))
			return NullptrError(MAXON_SOURCE_LOCATION);

		newItem.SetScript(scriptContent) iferr_return;
		newItem.Execute(debug) iferr_return;

		scriptOp->Remove();
		BaseList2D::Free(scriptOp);
	}

	return OK;
}

/// ----- WebSocket Messages ------ 

static Result<void> OnConnected(const NetworkWebSocketConnectionRef& webSocket, const DataDictionary& request)
{
	iferr_scope;

	WebSocketJsonCodeExchangeRef ce = WebSocketJsonCodeExchange().Create() iferr_return;
	BaseArray<WeakRef<const NetworkWebSocketConnectionRef>>* websockets = ce.GetWebSockets();
	if (MAXON_UNLIKELY(websockets == nullptr))
		return OK;

	websockets->Append(webSocket) iferr_return;

	return OK;
}

static void OnDisconnected(const NetworkWebSocketConnectionRef& webSocket)
{
	iferr_scope_handler
	{
		return;
	};

	WebSocketJsonCodeExchangeRef ce = WebSocketJsonCodeExchange().Create() iferr_return;
	BaseArray<WeakRef<const NetworkWebSocketConnectionRef>>* websockets = ce.GetWebSockets();
	if (MAXON_UNLIKELY(websockets == nullptr))
		return;

	Int32 cnt = -1;
	for (auto const& sockRef : *websockets)
	{
		cnt++;
		if (!sockRef)
			continue;

		const NetworkWebSocketConnectionRef sock = sockRef;

		if (sock == webSocket)
		{
			break;
		}
	}

	if (cnt != -1 && cnt < websockets->GetCount())
	{
		websockets->Erase(cnt) iferr_return;
	}

	return;
}

static Result<void> OnMessage(const NetworkWebSocketConnectionRef& webSocket, WEBSOCKET_OPCODE opCode, const BaseArray<Char>& data)
{
	iferr_scope;

	if (opCode != WEBSOCKET_OPCODE::TEXT)
		return OK;

	String inMsg = String(data.ToBlock());
	DataDictionary inData = JsonStringToDataDict(inMsg) iferr_return;
	if (inData.IsEmpty())
		return OK;

	Id action = GetActionId(inData);
	DataDictionary outDict; // To feed with value to send

	// Send CODEEXCHANGE::C4D2IDE::LOAD_SCRIPT_CONTENT
	if (action == CODEEXCHANGE::IDE2C4D::GET_SCRIPT_CONTENT)
	{
		GetScriptContent(inData, outDict) iferr_return;
	}
	
	else if (action == CODEEXCHANGE::IDE2C4D::LOAD_IN_SCRIPT_MANAGER)
	{
		LoadInScriptManager(inData, webSocket) iferr_return;
		return OK;
	}
	
	else if (action == CODEEXCHANGE::IDE2C4D::SET_SCRIPT_CONTENT)
	{
		SetScriptContent(inData) iferr_return;
		return OK;
	}
	
	else if (action == CODEEXCHANGE::IDE2C4D::EXECUTE_SCRIPT)
	{
		ExecuteScript(inData) iferr_return;
		return OK;
	}

	else if (action == CODEEXCHANGE::IDE2C4D::GET_PID)
	{
		outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::GET_PID) iferr_return;
		UInt pid = SystemProcessInterface::GetCurrentProcessId();
		outDict.Set(CODEEXCHANGE::VALUE, pid) iferr_return;
	}

	else if (action == CODEEXCHANGE::IDE2C4D::GET_PATH)
	{
		outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::GET_PATH) iferr_return;
		Url url = Application::GetUrl(APPLICATION_URLTYPE::STARTUP_DIR) iferr_return;
		String path = url.GetSystemPath() iferr_return;
		outDict.Set(CODEEXCHANGE::VALUE, path) iferr_return;
	}

	String out = DataDictToJsonString(outDict) iferr_return;
	webSocket.Send(out) iferr_return;

	return OK;
}

static Result<String> OnHandShake(const NetworkWebSocketConnectionRef& webSocket, const DataDictionary& request)
{
	iferr_scope;
	return ""_s;
}

/// ----- Interface implementation ------ 

class WebSocketJsonCodeExchangeImpl : public Component<WebSocketJsonCodeExchangeImpl, WebSocketJsonCodeExchangeInterface>
{
	/// This Implementation is not allowed to be re-used and it act as a singleton
	MAXON_COMPONENT(FINAL_SINGLETON);

private:
	/// Store the list of connection, returned by GetWebSockets()
	BaseArray<WeakRef<const NetworkWebSocketConnectionRef>> _websockets;

	/// Store if the WebSocket server is running, returned by IsRunning()
	Bool _isRunning = false;
	TimerRef _timer;

public:
	MAXON_METHOD Result<void> Start()
	{
		iferr_scope;
		if (g_wsServer)
		{
			Stop() iferr_return;
		}

		g_wsServer = NetworkWebSocketServerClass().Create() iferr_return;

		// create and start server
		auto localAddr = maxon::NetworkIpAddrPort(127, 0, 0, 1, GetPortFromPreference());
		g_wsServer.ObservableHandshake().AddObserver(OnHandShake) iferr_return;
		g_wsServer.ObservableConnected().AddObserver(OnConnected) iferr_return;
		g_wsServer.ObservableDisconnected().AddObserver(OnDisconnected) iferr_return;
		g_wsServer.ObservableMessage().AddObserver(OnMessage) iferr_return;

		g_wsServer.StartWebServer(localAddr, false, "c4d_py_code_exchange"_s) iferr_return;
		_isRunning = true;

		auto g_TimerFunction = [this]()
		{
			iferr_scope_handler
			{
				return;
			};
			String contentToSend;
			g_consolOutputBufferLock.Lock();
			contentToSend = g_consolOutputBuffer;
			g_consolOutputBuffer = ""_s;
			g_consolOutputBufferLock.Unlock();

			for (auto const& sockRef : _websockets)
			{
				if (!sockRef)
					continue;

				const NetworkWebSocketConnectionRef sock = sockRef;

				DataDictionary outDict;
				outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::CONSOLE) iferr_return;
				outDict.Set(CODEEXCHANGE::VALUE, contentToSend) iferr_return;

				String out = DataDictToJsonString(outDict) iferr_return;
				iferr (sock.Send(out))
					continue;
			}
		};

		_timer = TimerInterface::AddPeriodicTimer(maxon::Seconds(consoleOutputInterval), g_TimerFunction, maxon::JOBQUEUE_CURRENT) iferr_return;

		return OK;
	}

	MAXON_METHOD Result<void> Stop()
	{
		iferr_scope;
		if (_timer)
			_timer.CancelAndWait();

		if (g_wsServer)
		{
			// All connection must be closed or g_wsServer.StopWebServer() is waiting forever
			for (auto const& sockRef : _websockets)
			{
				if (!sockRef)
					continue;

				const NetworkWebSocketConnectionRef sock = sockRef;
				if (sock.GetState() == WEBSOCKETSTATE::CONNECTED)
				{
					sock.Close() iferr_return;
				}
			}

			_websockets.Flush();
			g_wsServer.StopWebServer() iferr_return;
			g_wsServer = nullptr;
		}

		_isRunning = false;
		return OK;
	}

	MAXON_METHOD InternedId GetLanguage() const
	{
		return CodeExchangeLanguageId::Python;
	}

	MAXON_METHOD String GetName() const
	{
		return "WebSocket Json"_s;
	}

	MAXON_METHOD Result<void> SendScriptToIDE(const PythonElementScriptRef& script) const
	{
		iferr_scope;

		for (auto const& sockRef : _websockets)
		{
			if (!sockRef)
				continue;

			const NetworkWebSocketConnectionRef sock = sockRef;

			DataDictionary outDict;
			outDict.Set(CODEEXCHANGE::ACTION, CODEEXCHANGE::C4D2IDE::SET_SCRIPT_CONTENT) iferr_return;
			outDict.Set(CODEEXCHANGE::SCRIPT_PATH, script.GetPath()) iferr_return;

			String code = script.GetScript() iferr_return;
			outDict.Set(CODEEXCHANGE::VALUE, code) iferr_return;

			String out = DataDictToJsonString(outDict) iferr_return;
			iferr (sock.Send(out))
				continue;
		}
		return OK;
	}

	MAXON_METHOD Result<void> SendConsoleOutput(const String& content)
	{
		iferr_scope;

		if (!_isRunning)
			return OK;

		Bool isConnected = false;
		for (auto const& sockWeakRef : _websockets)
		{
			if (!sockWeakRef)
				continue;

			const NetworkWebSocketConnectionRef sockRef = sockWeakRef;
			if (sockRef.GetState() == WEBSOCKETSTATE::CONNECTED)
			{
				isConnected = true;
				break;
			}
		}

		if (isConnected)
		{
			g_consolOutputBufferLock.Lock();
			g_consolOutputBuffer += content;
			g_consolOutputBufferLock.Unlock();
		}

		return OK;
	}

	MAXON_METHOD BaseArray<WeakRef<const NetworkWebSocketConnectionRef>>* GetWebSockets()
	{
		return &_websockets;
	}

	MAXON_METHOD Bool IsRunning()
	{
		return _isRunning;
	}
};


MAXON_COMPONENT_CLASS_REGISTER(WebSocketJsonCodeExchangeImpl, CodeExchanges, "net.maxonsdk.class.codeexchange.websocket_json");
}

