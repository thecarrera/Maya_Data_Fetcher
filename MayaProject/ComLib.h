#pragma once

#include <iostream>
#include <windows.h>
#include <vector>

#define THREADCOUNT 2
#define STSIZE sizeof(size_t)
#define UINTSIZE sizeof(UINT)

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
		if (handle)
		{
			ReleaseMutex(handle);
		}
	}
	void Lock(DWORD milliseconds = INFINITE)
	{
		if (handle)
		{
			WaitForSingleObject(handle, milliseconds);
		}
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

	HRESULT sendPulse(CONNECTION_TYPE type);

	HANDLE hFileMap				{};
	Mutex mutex					{};
	char* mData					{};

	size_t* messageCount		{};
	size_t* mSize				{};
	size_t* freeMemSize			{};
	size_t* head				{};
	size_t* tail				{};
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
		MSG_TYPE		msgId		{ MSG_TYPE::DUMMY };
		ATTRIBUTE_TYPE	attrID		{ ATTRIBUTE_TYPE::NONE };
		size_t			msgSeq		{};
		size_t			msgLength	{};
	};

	ComLib(const std::string& fileMapName, const DWORD& buffSize);
	~ComLib();

	bool peekExistingMessage();
	char* recv();

	Connection_Status* connectionStatus;
	HEADER header						{};
	Mutex mutex							{};
	HANDLE hFileMap						{};
	char* mData							{};

	size_t* mSize						{};
	size_t* freeMemSize					{};
	size_t* head						{};
	size_t* tail						{};
};