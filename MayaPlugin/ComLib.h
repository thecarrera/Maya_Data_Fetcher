#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include <wrl.h>

#define THREADCOUNT 2
#define STSIZE sizeof(size_t)

using namespace Microsoft::WRL;

class Mutex
{
private:
	HANDLE handle;

public:
	Mutex()
	{
		handle = CreateMutex(nullptr, false, "Global\\sharedMutex");

		if (!handle)
		{
			printf("CreateMutex error: %d\n", GetLastError());
			exit(0);
		}
	}
	~Mutex()
	{
		ReleaseMutex(handle);
	}
	void Lock(DWORD milliseconds = INFINITE)
	{
		WaitForSingleObject(handle, milliseconds);
	}
	void Unlock()
	{
		ReleaseMutex(handle);
	}
};


class Connection_Status
{
public:
	enum class CONNECTION_TYPE
	{
		DUMMY,
		CONNECTED,
		ALIVE,
		DISCONNECTED,
		NOT_SENDER,
		CONFIRMATION
	};

	Connection_Status(const std::string& fileMapName, const DWORD& buffSize);
	~Connection_Status();

	bool peekExistingMessage();
	HRESULT checkConnection(CONNECTION_TYPE& type);

	HANDLE hFileMap						{};
	Mutex mutex							{};
	char* mData							{};

	size_t* messageCount				{};
	size_t* mSize						{};
	size_t* freeMemSize					{};
	size_t* head						{};
	size_t* tail						{};
};

class ComLib
{
public:
	enum class MSG_TYPE {
		DUMMY,
		MESSAGE,
		ACTIVECAM,
		RENAME,
		ALLOCATE,
		DEALLOCATE,
		ADDVALUES,
		UPDATEVALUES,
		CHANGEVALUES,
		REMOVEVALUES,
		UNDO,
		REDO
	};

	// Extended for every attribute collected! 
	enum class ATTRIBUTE_TYPE
	{
		NONE,
		OFFSTART,
		OFFEND,
		ATTRST,
		ATTREND,
		ERR,
		TERMINATION,
		NODE,
		NODENAME,
		NODEUUID,
		PARENT,
		MATRIX,
		PROJMATRIX,
		TEXPATH,
		SHCOLOR,
		SHCOLORMAP,
		SHAMBIENTCOLOR,
		SHAMBIENTCOLORMAP,
		SHTRANSPARANCY,
		SHTRANSPARANCYMAP,
		SHNORMALMAP,
		BUMPSHADER,
		BUMPTEXTURE,
		SESURFACE,
		FACES,
		VERTEX,
		VERTEXID,
		//NORMAL,
		//NORMALID,
		//UVSETS,
		//UVSET,
		//UV,
		//UVID,
		MESHTRANSFORMER,
		MESHSHADERS,
		POINTINTENSITY
	};

	struct HEADER
	{
		MSG_TYPE		msgId			{ MSG_TYPE::DUMMY };
		ATTRIBUTE_TYPE	attrID			{ ATTRIBUTE_TYPE::NONE };
		size_t			msgSeq			{};
		size_t			msgLength		{};
	};

	ComLib(const std::string& fileMapName, const DWORD& buffSize);
	~ComLib();
	void reset();

	bool send();
	void addToPackage(char* msg, const MSG_TYPE msgType, const ATTRIBUTE_TYPE attributeType, const size_t length);

	Connection_Status* connectionStatus;
	HANDLE hFileMap						{};
	Mutex mutex							{};
	HEADER header						{};
	char* mData							{};

	size_t* messageCount				{};
	size_t* mSize						{};
	size_t* freeMemSize					{};
	size_t* head						{};
	size_t* tail						{};

	std::vector<char> package			{};
	size_t packageSize					{};
};