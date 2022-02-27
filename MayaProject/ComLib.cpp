#include "ComLib.h"

ComLib::ComLib(const std::string& fileMapName, const DWORD& buffSize)
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

		this->mSize = reinterpret_cast<size_t*>(data) + 1;
		this->freeMemSize = this->mSize + 1;
		this->head = this->freeMemSize + 1;
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
		this->mSize = reinterpret_cast<size_t*>(data) + 1;
		this->freeMemSize = this->mSize + 1;
		this->head = this->freeMemSize + 1;
		this->tail = this->head + 1;
		this->mData = reinterpret_cast<char*>(this->tail + 1);

		mutex.Lock();
		*this->mSize = buffSize;
		*this->freeMemSize = buffSize;
		*this->head = 0;
		*this->tail = 0;
		mutex.Unlock();
	}

	this->connectionStatus = new Connection_Status("connection", (200ULL << 12ULL));
}
ComLib::~ComLib()
{
	if (this->hFileMap)
	{
		mutex.Lock();
		delete this->connectionStatus;
		UnmapViewOfFile((LPCVOID)mData);
		CloseHandle(hFileMap);
		mutex.Unlock();
	}
}

char* ComLib::recv()
{
	if (this->hFileMap)
	{
		mutex.Lock();
		size_t freeMemSize {*this->freeMemSize};
		size_t memSize {*this->mSize};
		mutex.Unlock();
		
		std::cout << "freeMemSize: " << freeMemSize << std::endl;
		std::cout << "memSize: " << memSize << std::endl;

		if (freeMemSize < memSize)
		{
			size_t messageLength {reinterpret_cast<HEADER*>(this->mData + *this->tail)->msgLength};
			//std::cout << "messageLength: " << messageLength << std::endl;
			
			char* msg = this->mData + *this->tail;
			*this->tail += sizeof(HEADER) + messageLength;
			*this->freeMemSize += sizeof(HEADER) + messageLength;

			if (*this->tail == *this->mSize) 
			{
				*this->tail = 0;
			}

			std::cout << "New freeMemSize: " << *this->freeMemSize << std::endl;
			std::cout << "tail: " << *this->tail << std::endl << std::endl;
			return msg;
		}
	}
	return nullptr;
}

bool ComLib::peekExistingMessage()
{
	if (this->hFileMap)
	{
		mutex.Lock();
		size_t* messageCount {this->mSize - 1};
 		mutex.Unlock();
		if (*messageCount > 0)
		{
			std::cout << "messageCount: " << *messageCount << std::endl;
			return 1;
		}
	}
	return 0;
}

