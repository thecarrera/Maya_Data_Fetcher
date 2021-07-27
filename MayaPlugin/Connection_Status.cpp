#include "ComLib.h"
#include <maya/MGlobal.h>

Connection_Status::Connection_Status(const std::string& fileMapName, const DWORD& buffSize)
{
	this->hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		NULL,
		buffSize + sizeof(size_t) * 4,
		fileMapName.c_str()
	);
	if (!hFileMap)
	{
		//Fatal error
		std::cout << "Fatal error: couldn't create filemap" << std::endl;
		exit(-1);
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		//handle sharing
		char* data{
			static_cast<char*>(MapViewOfFile(
				hFileMap,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				0
			))
		};

		if (!data)
		{
			std::cout << "View map couldn't be created!" << std::endl;
			exit(-1);
		}

		this->messageCount = reinterpret_cast<size_t*>(data);
		this->mSize = messageCount + 1;
		this->freeMemSize = mSize + 1;
		this->head = freeMemSize + 1;
		this->tail = this->head + 1;
		this->mData = reinterpret_cast<char*>(this->tail + 1);


	}
	else
	{
		char* data{
			static_cast<char*>(MapViewOfFile(
				hFileMap,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				0
			))
		};

		if (!data)
		{
			std::cout << "View map couldn't be created!" << std::endl;
			exit(-1);
		}

		this->messageCount = reinterpret_cast<size_t*>(data);
		this->mSize = messageCount + 1;
		this->freeMemSize = mSize + 1;
		this->head = freeMemSize + 1;
		this->tail = this->head + 1;
		this->mData = reinterpret_cast<char*>(this->tail + 1);

		mutex.Lock();
		*this->messageCount = 0;
		*this->mSize = buffSize;
		*this->freeMemSize = buffSize;
		*this->head = 0;
		*this->tail = 0;
		mutex.Unlock();
	}
}
Connection_Status::~Connection_Status()
{
	if (this->hFileMap)
	{
		UnmapViewOfFile((LPCVOID)this->mData);
		CloseHandle(this->hFileMap);
	}
}

bool Connection_Status::peekExistingMessage()
{
	if (this->hFileMap)
	{
		mutex.Lock();
		size_t messageCount{ *(this->mSize - 1) };
		mutex.Unlock();
		if (messageCount > 0)
		{
			MGlobal::displayInfo("message Exist");
			return 1;
		}
	}
	return 0;
}
HRESULT Connection_Status::checkConnection(CONNECTION_TYPE& type)
{
	MString db{};
	if (this->hFileMap)
	{
		//MGlobal::displayInfo("A");
		size_t freeMemSize {*this->freeMemSize};
		size_t memSize {*this->mSize};

		//db = freeMemSize;
		//MGlobal::displayInfo(db);
		//db = memSize;
		//MGlobal::displayInfo(db);

		if (freeMemSize < memSize)
		{
			CONNECTION_TYPE* msg {reinterpret_cast<CONNECTION_TYPE*>(this->mData + *this->tail)};
			this->mutex.Lock();
			*this->messageCount -= 1;
			*this->tail += sizeof(CONNECTION_TYPE);
			*this->freeMemSize += sizeof(CONNECTION_TYPE);

			//MGlobal::displayInfo("messageCount:");
			//db = static_cast<unsigned int>(*this->messageCount);
			//MGlobal::displayInfo(db);


			if (*this->tail == *this->mSize)
			{ 
				*this->tail = 0;
			}
			this->mutex.Unlock();
			memcpy(&type, msg, sizeof(CONNECTION_TYPE));
			return S_OK;
		}
	}
	return E_FAIL;
}