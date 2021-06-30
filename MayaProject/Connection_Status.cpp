#include "ComLib.h"

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
		UnmapViewOfFile((LPCVOID)mData);
		CloseHandle(hFileMap);
	}
}

HRESULT Connection_Status::sendPulse(CONNECTION_TYPE type)
{
	if (this->hFileMap)
	{
		size_t CONNECTION_SIZE {sizeof(CONNECTION_TYPE)};

		if (*this->freeMemSize > CONNECTION_SIZE)
		{
			if (*this->freeMemSize - (CONNECTION_SIZE *2) <= *this->head)
			{
				CONNECTION_TYPE dummy {CONNECTION_TYPE::DUMMY};
				memcpy(this->mData + *this->head, &dummy, CONNECTION_SIZE);
				
				this->mutex.Lock();
				*this->messageCount += 1;
				*this->head = 0;
				*this->freeMemSize -= *this->mSize - *this->head;
				this->mutex.Unlock();

				return E_FAIL;
			}
			else
			{
				memcpy(this->mData + *this->head, &type, CONNECTION_SIZE);
				*this->messageCount += 1;	
				*this->head += CONNECTION_SIZE;
				*this->freeMemSize -= CONNECTION_SIZE;
				return S_OK;
			}
		}
	}
	return E_FAIL;
}