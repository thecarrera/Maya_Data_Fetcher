#pragma once

#include <iostream>
#include <windows.h>
#include <vector>

#define THREADCOUNT 2
#define STSIZE sizeof(size_t)

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
			getchar();
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
		REMOVEVALUES,
		UNDO,
		REDO
	};

	// Extended for every attribute collected! 
	enum class ATTRIBUTE_TYPE
	{
		NONE,
		EXSTART,
		EXEND,
		ERR,
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
		NORMALMAP,
		BUMPSHADER,
		BUMPTEXTURE,
		SESURFACE,
		VERTEX,
		VERTEXID,
		//NORMAL,
		//NORMALID,
		//UVSETS,
		//UVSET,
		//UV,
		//UVID,
		MESHSHADERS,
		POINTINTENSITY
	};

	struct Header
	{
		MSG_TYPE		msgId		{ MSG_TYPE::DUMMY };
		ATTRIBUTE_TYPE	attrID		{ ATTRIBUTE_TYPE::NONE };
		size_t			msgSeq		{};
		size_t			msgLength	{};
	};

	// create a ComLib
	ComLib(const std::string& fileMapName, const DWORD& buffSize);
	/* disconnect and destroy all resources */
	~ComLib();

	// init and check status
	bool connect();
	bool isConnected();

	// returns "true" if data was sent successfully.
	// false if for any reason the data could not be sent.
	// Remember to delete msg pointers?
	bool send(const void* msg, const MSG_TYPE msgType, const ATTRIBUTE_TYPE attrType, const size_t length);

	char* recv();

	bool peekExistingMessage();
	void calcFreeMem();

	HANDLE hFileMap				{};
	char* mData					{};
	bool exits					{false};
	size_t mSize			{};
	size_t freeMemSize	{};

	Header header	{};
	size_t* head	{};
	size_t* tail	{};
};