#include "ComLib.h"

ComLib::ComLib(const std::string& fileMapName, const DWORD& buffSize) : mSize(buffSize)
{
	this->hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		NULL,
		buffSize + sizeof(size_t) * 2,
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

		this->head = reinterpret_cast<size_t*>(data);
		this->tail = this->head + 1;
		this->mData = reinterpret_cast<char*>(this->tail) + sizeof(size_t);
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
		this->head = reinterpret_cast<size_t*>(data);
		this->tail = this->head + 1;
		this->mData = reinterpret_cast<char*>(this->tail) + sizeof(size_t);
		*this->head = 0;
		*this->tail = 0;
	}
}
ComLib::~ComLib()
{
	UnmapViewOfFile((LPCVOID)mData);
	CloseHandle(hFileMap);
}

bool ComLib::connect()
{
	return false;
}
bool ComLib::isConnected()
{
	return false;
}

bool ComLib::send(const void* msg, const MSG_TYPE msgType, const ATTRIBUTE_TYPE attrType, const size_t length)
{
	this->calcFreeMem();

	size_t msgSize = sizeof(Header) + length;
	size_t blockCount = ceil(msgSize / 64.f);
	size_t pad = (blockCount * 64) - msgSize;
	size_t totalBlockSize = msgSize + pad;

	if (this->freeMemSize > totalBlockSize)
	{
		if (this->mSize - msgSize <= *this->head)
		{
			if (*this->tail == 0)
			{
				return send(msg, msgType, attrType, length);
			}
			else
			{
				ComLib::Header h;

				h.msgLength = mSize - *this->head;

				memcpy(this->mData + *this->head, &h, sizeof(Header));
				*this->head = 0;

				return send(msg, msgType, attrType, length);
			}
		}
		else
		{
			header.msgId = msgType;
			header.attrID = attrType;
			header.msgSeq++;
			header.msgLength = length + pad;

			memcpy(this->mData + *this->head, &header, sizeof(Header));
			*this->head += sizeof(Header);

			memcpy(this->mData + *this->head, msg, header.msgLength);
			this->head += header.msgLength;

			if (*this->head == mSize)
			{
				*this->head = 0;
			}
			return true;
		}
	}
	else
	{
		return false;
	}
}

char* ComLib::recv()
{
	size_t messageLength{ reinterpret_cast<Header*>(this->mData + *this->tail)->msgLength };
	char* msg{ this->mData + *this->tail };
	*this->tail += sizeof(Header) + messageLength;
	if (*this->tail == this->mSize) 
	{
		*this->tail = 0;
	}
	return msg;
}

bool ComLib::peekExistingMessage()
{
	if (*this->head == *this->tail)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void ComLib::calcFreeMem()
{
	if (*this->head == *this->tail)
	{
		this->freeMemSize = mSize;
	}
	else if (*this->head > *this->tail)
	{
		size_t temp1 = mSize - *this->head;

		this->freeMemSize = temp1 + *this->tail;
	}
	else
	{
		this->freeMemSize = *this->tail - *this->head;
	}
}

