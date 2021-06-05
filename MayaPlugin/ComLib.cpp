#include "ComLib.h"
#include <maya\MGlobal.h>

ComLib::ComLib(const std::string& fileMapName, const size_t& buffSize) : mSize(buffSize)
{
	this->hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		NULL,
		mSize + sizeof(size_t) * 2,
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

	int msgSize = sizeof(Header) + length;
	int blockCount = ceil(msgSize / 64.f);
	int pad = (blockCount * 64) - msgSize;
	int totalBlockSize = msgSize + pad;

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
			return true;
		}
	}
	else
	{
		return send(msg, msgType, attrType, length);
	}
}

//bool ComLib::recv(char* msg, size_t& length)
//{
//	//test1
//	return false;
//}

void ComLib::calcFreeMem()
{
	if (*this->head == *this->tail)
	{
		this->freeMemSize = mSize;
	}
	else if (*this->head > *this->tail)
	{
		int temp1 = mSize - *this->head;

		this->freeMemSize = temp1 + *this->tail;
	}
	else
	{
		this->freeMemSize = *this->tail - *this->head;
	}
}

