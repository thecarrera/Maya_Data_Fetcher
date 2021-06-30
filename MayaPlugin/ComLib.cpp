#include "ComLib.h"
#include <maya\MGlobal.h>

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

		this->messageCount	= reinterpret_cast<size_t*>(data);
		this->mSize			= this->messageCount + 1;
		this->freeMemSize	= this->mSize + 1;
		this->head			= this->freeMemSize + 1;
		this->tail			= this->head + 1;
		this->mData			= reinterpret_cast<char*>(this->tail + 1);
			
		mutex.Lock();
		*this->messageCount = 0;
		mutex.Unlock();
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

	this->connectionStatus = new Connection_Status("connection", (200ULL << 12ULL));
}
ComLib::~ComLib()
{
	if (this->hFileMap)
	{
		mutex.Lock();
		delete this->connectionStatus;
		UnmapViewOfFile((LPCVOID)this->mData);
		CloseHandle(this->hFileMap);
		mutex.Unlock();
	}
}

void ComLib::reset()
{
	this->mutex.Lock();
	this->header.msgSeq = 0;
	*this->head = 0;
	*this->tail = 0;
	*this->freeMemSize = *this->mSize;
	*this->messageCount = 0;
	this->mutex.Unlock();
}

bool ComLib::send()
{
	MString db{};
	size_t blockCount {static_cast<size_t>(ceil((this->packageSize + sizeof(HEADER))/ 64.f))};
	size_t pad = (blockCount * 64) - (this->packageSize + sizeof(HEADER));
	size_t totalBlockSize = this->packageSize + sizeof(HEADER) + pad;

	db = "blockCount: ";
	db += static_cast<unsigned int>(blockCount);
	MGlobal::displayInfo(db);
	db = "pad: ";
	db += static_cast<unsigned int>(pad);
	MGlobal::displayInfo(db);
	db = "totalBlockSize: ";
	db += static_cast<unsigned int>(totalBlockSize);
	MGlobal::displayInfo(db);


	if (*this->freeMemSize > totalBlockSize)
	{
		if (*this->mSize - totalBlockSize <= *this->head)
		{
			MGlobal::displayInfo("9.1");
			this->header.msgId = MSG_TYPE::DUMMY;
			this->header.attrID = ATTRIBUTE_TYPE::NONE;
			this->header.msgSeq++;
			this->header.msgLength = *this->mSize - *this->head;

			memcpy(this->mData + *this->head, &this->header, sizeof(HEADER));
			
			this->mutex.Lock();
			*this->head = 0;
			*this->freeMemSize -= header.msgLength;

			db = "messageCount: ";
			db += static_cast<unsigned int>(*this->messageCount);
			MGlobal::displayInfo(db);
			db = "freeMemSize: ";
			db = *this->freeMemSize;
			MGlobal::displayInfo(db);

			this->mutex.Unlock();

			return false;
		}
		else
		{
			if (this->packageSize > 0)
			{
				MGlobal::displayInfo("9.2");

				this->header.msgId = MSG_TYPE::DUMMY;
				this->header.attrID = ATTRIBUTE_TYPE::NONE;
				this->header.msgSeq++;
				this->header.msgLength = pad;

				this->mutex.Lock();
				memcpy(this->mData + *this->head, this->package.data(), this->packageSize);
				*this->head += this->packageSize;
				memcpy(this->mData + *this->head, &this->header, sizeof(HEADER));
				*this->head += sizeof(HEADER) + pad;

				db = "packageSize: ";
				db += static_cast<unsigned int>(this->packageSize);
				MGlobal::displayInfo(db);
				db = "header + pad: ";
				db += static_cast<unsigned int>(sizeof(header) + pad);
				MGlobal::displayInfo(db);

				*this->freeMemSize -= totalBlockSize;
				*this->messageCount += 1;
				this->package.clear();
				this->packageSize = 0;
				this->mutex.Unlock();
			}
			return true;
		}
	}
	MGlobal::displayInfo("Buffer full, waiting for other application!");
	return false;
}
void ComLib::addToPackage(char* msg, const MSG_TYPE msgType, const ATTRIBUTE_TYPE attrType, const size_t length)
{
	MString debugString {};
	std::vector<char> tempMsg {};
	size_t tempMessageOffset {};

	size_t blockCount{ static_cast<size_t>(ceil((length + sizeof(HEADER)) / 64.f)) };
	size_t pad = (blockCount * 64) - (length + sizeof(HEADER));
	size_t totalBlockSize = length + sizeof(HEADER) + pad;

	if (this->packageSize > 0)
	{
		tempMsg.resize(this->packageSize + length + sizeof(HEADER));
		memcpy(tempMsg.data(), this->package.data(), this->packageSize);
		tempMessageOffset += this->packageSize;
	}
	else
	{
		tempMsg.resize(sizeof(HEADER) + length);
	}

	debugString = packageSize;
	MGlobal::displayInfo(debugString);

	this->header.msgId = msgType;
	this->header.attrID = attrType;
	this->header.msgSeq++;
	this->header.msgLength = length + pad;
	
	debugString = "tempOffset: ";
	debugString += static_cast<unsigned int>(tempMessageOffset);
	MGlobal::displayInfo(debugString);
	debugString = "message Length: ";
	debugString += static_cast<unsigned int>(length);
	MGlobal::displayInfo(debugString);
	debugString = "tempMsg size: ";
	debugString += static_cast<unsigned int>(tempMsg.size());
	MGlobal::displayInfo(debugString);


	memcpy(tempMsg.data() + tempMessageOffset, &header, sizeof(HEADER));
	tempMessageOffset += sizeof(HEADER);

	if (length > 0)
	{
		memcpy(tempMsg.data() + tempMessageOffset, msg, length);
		tempMessageOffset += length;
	}
	

	debugString = tempMsg.size();
	MGlobal::displayInfo(debugString);

	this->mutex.Lock();
	this->package.resize(this->packageSize + totalBlockSize);
	memcpy(this->package.data(), tempMsg.data(), tempMessageOffset);
	this->packageSize = this->packageSize + totalBlockSize;
	this->mutex.Unlock();
	debugString = "Package Size: ";
	debugString += static_cast<unsigned int>(packageSize);
	MGlobal::displayInfo(debugString);

}

