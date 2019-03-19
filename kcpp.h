#pragma once

#include <functional>
#include <memory>
#include <deque>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <vector>
#include <assert.h>
#include "ikcp.h"


// "License": Public Domain
// I, Mathias Panzenbà¸£à¸–ck, place this file hereby into the public domain. Use it at your own risk for whatever you like.
// In case there are jurisdictions that don't support putting things in the public domain you can also consider it to
// be "dual licensed" under the BSD, MIT and Apache licenses, if you want to. This code is trivial anyway. Consider it
// an example on how to get the endian conversion functions on different platforms.


#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#	define __WINDOWS__

#endif

#if defined(__linux__) || defined(__CYGWIN__)

#	include <endian.h>

#elif defined(__APPLE__)

#	include <libkern/OSByteOrder.h>

#	define htobe16(x) OSSwapHostToBigInt16(x)
#	define htole16(x) OSSwapHostToLittleInt16(x)
#	define be16toh(x) OSSwapBigToHostInt16(x)
#	define le16toh(x) OSSwapLittleToHostInt16(x)

#	define htobe32(x) OSSwapHostToBigInt32(x)
#	define htole32(x) OSSwapHostToLittleInt32(x)
#	define be32toh(x) OSSwapBigToHostInt32(x)
#	define le32toh(x) OSSwapLittleToHostInt32(x)

#	define htobe64(x) OSSwapHostToBigInt64(x)
#	define htole64(x) OSSwapHostToLittleInt64(x)
#	define be64toh(x) OSSwapBigToHostInt64(x)
#	define le64toh(x) OSSwapLittleToHostInt64(x)

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#elif defined(__OpenBSD__)

#	include <sys/endian.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#	include <sys/endian.h>

#	define be16toh(x) betoh16(x)
#	define le16toh(x) letoh16(x)

#	define be32toh(x) betoh32(x)
#	define le32toh(x) letoh32(x)

#	define be64toh(x) betoh64(x)
#	define le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#	include <winsock2.h>
//#	include <sys/param.h>

#if !defined(BYTE_ORDER) && defined(__BYTE_ORDER)
#define BYTE_ORDER __BYTE_ORDER

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif /* LITTLE_ENDIAN */

#ifndef BIG_ENDIAN
#define BIG_ENDIAN __LITTLE_ENDIAN
#endif /* BIG_ENDIAN */
#endif /* BYTE_ORDER */

#	if BYTE_ORDER == LITTLE_ENDIAN

#		define htobe16(x) htons(x)
#		define htole16(x) (x)
#		define be16toh(x) ntohs(x)
#		define le16toh(x) (x)

#		define htobe32(x) htonl(x)
#		define htole32(x) (x)
#		define be32toh(x) ntohl(x)
#		define le32toh(x) (x)

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

#ifndef ntohll
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#		define htobe64(x) htonll(x)
#		define htole64(x) (x)
#		define be64toh(x) ntohll(x)
#		define le64toh(x) (x)

#	elif BYTE_ORDER == BIG_ENDIAN

/* that would be xbox 360 */
#		define htobe16(x) (x)
#		define htole16(x) __builtin_bswap16(x)
#		define be16toh(x) (x)
#		define le16toh(x) __builtin_bswap16(x)

#		define htobe32(x) (x)
#		define htole32(x) __builtin_bswap32(x)
#		define be32toh(x) (x)
#		define le32toh(x) __builtin_bswap32(x)

#		define htobe64(x) (x)
#		define htole64(x) __builtin_bswap64(x)
#		define be64toh(x) (x)
#		define le64toh(x) __builtin_bswap64(x)

#	else

#		error byte order not supported

#	endif

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#else

#	error platform not supported

#endif


namespace kcpp
{

// a light weight buffer.
// thx to chensuo, modify on muduo::net::Buffer and make it safe to prepend data of any length.

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buf
{
public:
	static const size_t kCheapPrepend = 1024;
	static const size_t kInitialSize = 512;

	explicit Buf(size_t initialSize = kInitialSize)
		:
		buffer_(kCheapPrepend + initialSize),
		readerIndex_(kCheapPrepend),
		writerIndex_(kCheapPrepend)
	{
		assert(readableBytes() == 0);
		assert(writableBytes() == initialSize);
		assert(prependableBytes() == kCheapPrepend);
	}

	// implicit copy-ctor, move-ctor, dtor and assignment are fine
	// NOTE: implicit move-ctor is added in g++ 4.6

	void swap(Buf& rhs)
	{
		buffer_.swap(rhs.buffer_);
		std::swap(readerIndex_, rhs.readerIndex_);
		std::swap(writerIndex_, rhs.writerIndex_);
	}

	size_t readableBytes() const
	{ return writerIndex_ - readerIndex_; }

	size_t writableBytes() const
	{ return buffer_.size() - writerIndex_; }

	size_t prependableBytes() const
	{ return readerIndex_; }

	const char* peek() const
	{ return begin() + readerIndex_; }

	const char* findCRLF() const
	{
		// FIXME: replace with memmem()?
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? NULL : crlf;
	}

	const char* findCRLF(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		// FIXME: replace with memmem()?
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? NULL : crlf;
	}

	const char* findEOL() const
	{
		const void* eol = memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}

	const char* findEOL(const char* start) const
	{
		assert(peek() <= start);
		assert(start <= beginWrite());
		const void* eol = memchr(start, '\n', beginWrite() - start);
		return static_cast<const char*>(eol);
	}

	// retrieve returns void, to prevent
	// std::string str(retrieve(readableBytes()), readableBytes());
	// the evaluation of two functions are unspecified
	void retrieve(size_t len)
	{
		assert(len <= readableBytes());
		if (len < readableBytes())
		{
			readerIndex_ += len;
		}
		else
		{
			retrieveAll();
		}
	}

	void retrieveUntil(const char* end)
	{
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}

	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}

	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}

	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}

	void retrieveAll()
	{
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	std::string retrieveAllAsString()
	{
		return retrieveAsString(readableBytes());
	}

	std::string retrieveAsString(size_t len)
	{
		assert(len <= readableBytes());
		std::string result(peek(), len);
		retrieve(len);
		return result;
	}

	void append(const std::string& str)
	{
		append(str.data(), str.size());
	}

	void append(const char* /*restrict*/ data, size_t len)
	{
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	void append(const void* /*restrict*/ data, size_t len)
	{
		append(static_cast<const char*>(data), len);
	}

	void ensureWritableBytes(size_t len)
	{
		if (writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}

	char* beginWrite()
	{ return begin() + writerIndex_; }

	const char* beginWrite() const
	{ return begin() + writerIndex_; }

	void hasWritten(size_t len)
	{
		assert(len <= writableBytes());
		writerIndex_ += len;
	}

	void unwrite(size_t len)
	{
		assert(len <= readableBytes());
		writerIndex_ -= len;
	}

	///
	/// Append int64_t using network endian
	///
	void appendInt64(int64_t x)
	{
		int64_t be64 = htobe64(x);
		append(&be64, sizeof be64);
	}

	///
	/// Append int32_t using network endian
	///
	void appendInt32(int32_t x)
	{
		int32_t be32 = htobe32(x);
		append(&be32, sizeof be32);
	}

	void appendInt16(int16_t x)
	{
		int16_t be16 = htobe16(x);
		append(&be16, sizeof be16);
	}

	void appendInt8(int8_t x)
	{
		append(&x, sizeof x);
	}

	///
	/// Read int64_t from network endian
	///
	/// Require: buf->readableBytes() >= sizeof(int32_t)
	int64_t readInt64()
	{
		int64_t result = peekInt64();
		retrieveInt64();
		return result;
	}

	///
	/// Read int32_t from network endian
	///
	/// Require: buf->readableBytes() >= sizeof(int32_t)
	int32_t readInt32()
	{
		int32_t result = peekInt32();
		retrieveInt32();
		return result;
	}

	int16_t readInt16()
	{
		int16_t result = peekInt16();
		retrieveInt16();
		return result;
	}

	int8_t readInt8()
	{
		int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}

	///
	/// Peek int64_t from network endian
	///
	/// Require: buf->readableBytes() >= sizeof(int64_t)
	int64_t peekInt64() const
	{
		assert(readableBytes() >= sizeof(int64_t));
		int64_t be64 = 0;
		::memcpy(&be64, peek(), sizeof be64);
		return be64toh(be64);
	}

	///
	/// Peek int32_t from network endian
	///
	/// Require: buf->readableBytes() >= sizeof(int32_t)
	int32_t peekInt32() const
	{
		assert(readableBytes() >= sizeof(int32_t));
		int32_t be32 = 0;
		::memcpy(&be32, peek(), sizeof be32);
		return be32toh(be32);
	}

	int16_t peekInt16() const
	{
		assert(readableBytes() >= sizeof(int16_t));
		int16_t be16 = 0;
		::memcpy(&be16, peek(), sizeof be16);
		return be16toh(be16);
	}

	int8_t peekInt8() const
	{
		assert(readableBytes() >= sizeof(int8_t));
		int8_t x = *peek();
		return x;
	}

	///
	/// Prepend int64_t using network endian
	///
	void prependInt64(int64_t x)
	{
		int64_t be64 = htobe64(x);
		prepend(&be64, sizeof be64);
	}

	///
	/// Prepend int32_t using network endian
	///
	void prependInt32(int32_t x)
	{
		int32_t be32 = htobe32(x);
		prepend(&be32, sizeof be32);
	}

	void prependInt16(int16_t x)
	{
		int16_t be16 = htobe16(x);
		prepend(&be16, sizeof be16);
	}

	void prependInt8(int8_t x)
	{
		prepend(&x, sizeof x);
	}

	void prepend(const std::string& str)
	{
		prepend(str.data(), str.size());
	}

	void prepend(const void* /*restrict*/ data, size_t len)
	{
		ensurePrependableBytes(len);
		readerIndex_ -= len;
		const char* d = static_cast<const char*>(data);
		std::copy(d, d + len, begin() + readerIndex_);
	}

	void ensurePrependableBytes(size_t len)
	{
		if (len > prependableBytes())
		{
			makeSpaceForPrepend(len);
		}
		assert(prependableBytes() >= len);
	}

	size_t internalCapacity() const
	{
		return buffer_.capacity();
	}

private:

	char* begin()
	{ return &*buffer_.begin(); }

	const char* begin() const
	{ return &*buffer_.begin(); }

	void makeSpace(size_t len)
	{
		if (writableBytes() + prependableBytes() < len + kCheapPrepend)
		{
			// FIXME: move readable data
			buffer_.resize(writerIndex_ + len);
		}
		else
		{
			// move readable data to the front, make space inside buffer
			assert(kCheapPrepend < readerIndex_);
			size_t readable = readableBytes();
			std::copy(begin() + readerIndex_,
				begin() + writerIndex_,
				begin() + kCheapPrepend);
			readerIndex_ = kCheapPrepend;
			writerIndex_ = readerIndex_ + readable;
			assert(readable == readableBytes());
		}
	}

	void makeSpaceForPrepend(size_t len)
	{
		if (len - readerIndex_ > writableBytes())
		{
			buffer_.resize(writerIndex_ + (len - readerIndex_));
		}
		// move readable data to the end
		size_t readable = readableBytes();
		std::copy(begin() + readerIndex_,
			begin() + writerIndex_,
			begin() + len);
		readerIndex_ = len;
		writerIndex_ = readerIndex_ + readable;
		assert(readable == readableBytes());
	}

private:
	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;
	static const char kCRLF[];
};



struct UserInputData
{
	UserInputData(char *data = nullptr, const int len = 0)
	{
		this->len_ = len;
		if (len > 0)
			this->data_ = data;
		else
			this->data_ = nullptr;
	}
	char* data_;
	int len_;
};

class KcpSession;
typedef std::shared_ptr<KcpSession> KcpSessionPtr;

typedef std::function<void(const void* pendingSendData, int pendingSendDataLen)> UserOutputFunction;
typedef std::function<UserInputData()> UserInputFunction;
typedef std::function<int64_t()> CurrentTimestampMsFunction;
typedef std::function<void(std::deque<std::string>* pendingSendDataDeque)> KcpSessionConnectionCallback;

enum TransmitModeE { kUnreliable = 88, kReliable };
enum RoleTypeE { kSrv, kCli };
enum ConnectionStateE { kConnecting, kConnected, kResetting, kReset };
enum PktTypeE { kSyn = 66, kAck, kPsh, kRst };


class Rdc
{
public:
	typedef std::function<void(Buf*, int&, int, PktTypeE)> RecvFuncion;
	Rdc(const UserOutputFunction& userOutputFunc, const RecvFuncion& rcvFunc)
		:
		userOutputFunc_(userOutputFunc), rcvFunc_(rcvFunc), nextSndSn_(0), nextRcvSn_(0),
		isThisRoundFinished_(true), on_(false), mss_(548)
	{}

	int Output(Buf* oBuf, PktTypeE pktType)
	{
		size_t curLen = oBuf->readableBytes();
		if (pktType == static_cast<PktTypeE>(kUnreliable))
		{
			size_t frgCnt = 0;
			if (curLen <= kUnreliableDataLenLimit + kFrgLen)
				frgCnt = 1;
			else
				frgCnt = (curLen + kUnreliableDataLenLimit - 1) / kUnreliableDataLenLimit;

			if (frgCnt >= kMaxFrgCnt)
			{
				oBuf->retrieveAll();
				return -1;
			}

			size_t curDataLen = 0;
			size_t curHeaderLen = frgCnt > 1 ? kUnreliableHeaderLen : (kUnreliableHeaderLen - kFrgLen);
			size_t curDataLenLimit = kMaxMSS - curHeaderLen;
			for (size_t i = 0; i < frgCnt; ++i)
			{
				curDataLen = curLen > curDataLenLimit ? curDataLenLimit : curLen;
				oBuf->prependInt16(static_cast<int16_t>(curDataLen));
				if (frgCnt > 1)
					oBuf->prependInt8(static_cast<int8_t>(frgCnt - i - 1));
				oBuf->prependInt8(static_cast<int8_t>(frgCnt));
				oBuf->prependInt32(nextSndSn_++);
				oBuf->prependInt8(static_cast<int8_t>(kUnreliable));
				outputPktDeque_.emplace_back(std::string(oBuf->peek(), curHeaderLen + curDataLen));
				oBuf->retrieve(curHeaderLen + curDataLen);
				curLen -= curDataLen;
			}
			assert(curLen == 0);
			assert(oBuf->readableBytes() == 0);

			HandleFlush(oBuf, frgCnt);
		}
		else /*if(pktType != kUnreliable)*/
		{
			oBuf->prependInt16(static_cast<int16_t>(curLen));
			oBuf->prependInt32(nextSndSn_++);
			oBuf->prependInt8(static_cast<int8_t>(pktType));
			outputPktDeque_.emplace_back(std::string(oBuf->peek(), kReliableHeaderLen + curLen));
			oBuf->retrieveAll();

			HandleFlush(oBuf);
		}
		return 0;
	}

	bool Input(Buf* userBuf, int& len, Buf* iBuf)
	{
		PktTypeE pktType = static_cast<PktTypeE>(0);
		int32_t rcvSn = 0;
		int8_t rcvFrgCnt = 0;
		int8_t rcvFrg = 0;
		int16_t dataLen = 0;

		bool hasDataLeftThisRound = ParsePkt(iBuf, pktType, rcvSn, rcvFrgCnt, rcvFrg, dataLen);
		if (hasDataLeftThisRound)
		{
			auto discardRcvedData = [&]() { iBuf->retrieve(dataLen); len = 0; };
			isThisRoundFinished_ = false;

			if (pktType == static_cast<PktTypeE>(kUnreliable))
			{
				if (rcvSn >= nextRcvSn_)
				{
					nextRcvSn_ = rcvSn + 1;

					if (rcvFrgCnt == 1)
						rcvFunc_(userBuf, len, dataLen, pktType);
					else
					{
						if (rcvFrg != 0)
						{
							bool isHeadFrg = (rcvFrg == rcvFrgCnt - 1);
							if (isHeadFrg)
								inputFrgMap_.clear();

							if (isHeadFrg || inputFrgMap_.find(rcvSn - 1) != inputFrgMap_.end())
								inputFrgMap_.emplace(std::make_pair(
									rcvSn, std::string(iBuf->peek(), dataLen)));

							discardRcvedData();
						}
						else if (rcvFrg == 0)
						{
							if (inputFrgMap_.find(rcvSn - 1) != inputFrgMap_.end())
							{
								int sumDataLen = dataLen;
								for (int sn = rcvSn - 1; sn >= rcvSn - rcvFrgCnt + 1; --sn)
								{
									iBuf->prepend(inputFrgMap_[sn]);
									sumDataLen += static_cast<int>(inputFrgMap_[sn].size());
								}
								rcvFunc_(userBuf, len, sumDataLen, pktType);
							}
							else
								discardRcvedData();
						}
					} /*if (rcvFrgCnt > 1)*/
				}
				else if (rcvSn < nextRcvSn_)
					discardRcvedData();
			}
			else /*if (pktType != kUnreliable)*/
			{
				if (rcvSn >= nextRcvSn_)
				{
					nextRcvSn_ = rcvSn + 1;
					rcvFunc_(userBuf, len, dataLen, pktType);
				}
				else
					discardRcvedData();
			}
		}
		else if (!hasDataLeftThisRound)
		{
			iBuf->retrieveAll();
			isThisRoundFinished_ = true;
			len = 0;
		}
		return hasDataLeftThisRound;
	}

	bool IsThisRoundFinished() const { return isThisRoundFinished_; }

	void Switch(bool on) { on_ = on; }

	void SetMTU(size_t mtu)
	{ assert(mtu - 28 <= kMaxMSS); mss_ = mtu - 28; }

private:

	void FlushOutputBuffer(Buf* oBuf)
	{
		userOutputFunc_(oBuf->peek(), static_cast<int>(oBuf->readableBytes()));
		oBuf->retrieveAll();
	}

	void HandleFlush(Buf* oBuf, size_t frgCnt = 1)
	{
		if (frgCnt > 1)
		{
			for (size_t i = frgCnt; i >= 1; --i)
			{
				auto curIt = outputPktDeque_.end() - i;
				if (curIt->size() >= mss_)
				{
					oBuf->prepend(*curIt);
					FlushOutputBuffer(oBuf);
					outputPktDeque_.erase(curIt);
				}
				else
				{
					assert(i == 1);
					HandleDynamicRdc(oBuf, *curIt);
				}
			}
		}
		else if (frgCnt == 1)
		{
			HandleDynamicRdc(oBuf, outputPktDeque_.back());
		}
	}

	void HandleDynamicRdc(Buf* oBuf, const std::string& pendingSndData)
	{
		// PrependPrePktAndFlush(oBuf); return;

		if (on_)
			PrependPrePktAndFlush(oBuf);
		else
		{
			oBuf->prepend(pendingSndData);
			FlushOutputBuffer(oBuf);

			if (pendingSndData.size() >= mss_)
				outputPktDeque_.pop_back();

			size_t sumPktLen = 0;
			for (auto it = outputPktDeque_.end() - 1; it != outputPktDeque_.begin(); --it)
			{
				sumPktLen += it->size();
				if (sumPktLen > mss_)
				{
					outputPktDeque_.erase(outputPktDeque_.begin(), it);
					break;
				}
			}
		}
	}

	void PrependPrePktAndFlush(Buf* oBuf)
	{
		for (size_t i = 1; i <= outputPktDeque_.size(); ++i)
		{
			auto curIt = outputPktDeque_.end() - i;
			bool isFront = curIt == outputPktDeque_.begin();
			oBuf->prepend(*curIt);

			size_t prePktDataLen = 0;
			if (!isFront)
				prePktDataLen = (curIt - 1)->size();

			if ((oBuf->readableBytes() + prePktDataLen >= mss_) || isFront)
			{
				if (oBuf->readableBytes() + prePktDataLen >= mss_)
					outputPktDeque_.erase(outputPktDeque_.begin(), curIt);
				FlushOutputBuffer(oBuf);
				break;
			}
		}
	}

	bool ParsePkt(Buf* iBuf, PktTypeE &pktType, int32_t &rcvSn,
		int8_t &rcvFrgCnt, int8_t &rcvFrg, int16_t &dataLen) const
	{
		bool hasDataLeftThisRound = false;
		if (iBuf->readableBytes() >= kPktTypeLen + kSnLen + kDataLen)
		{
			pktType = static_cast<PktTypeE>(iBuf->readInt8());
			rcvSn = iBuf->readInt32();
			auto checkDataLenFunc = [&](TransmitModeE tm, bool noFrg = false) {
				if (iBuf->readableBytes() >= kDataLen)
				{
					dataLen = iBuf->readInt16();
					size_t curDataLenLimit = 0;
					if (tm == kReliable)
						curDataLenLimit = kReliableDataLenLimit;
					else
					{
						curDataLenLimit = kUnreliableDataLenLimit;
						if (noFrg)
							curDataLenLimit = kUnreliableDataLenLimit + kFrgLen;
					}
					if (dataLen <= static_cast<int16_t>(curDataLenLimit))
						hasDataLeftThisRound = iBuf->readableBytes() >= static_cast<size_t>(dataLen);
				}};
			if (pktType == static_cast<PktTypeE>(kUnreliable))
			{
				rcvFrgCnt = iBuf->readInt8();
				if (rcvFrgCnt > 1 && static_cast<size_t>(rcvFrgCnt) <= kMaxFrgCnt)
				{
					rcvFrg = iBuf->readInt8();
					checkDataLenFunc(kUnreliable);
				}
				else if (rcvFrgCnt == 1)
					checkDataLenFunc(kUnreliable, true);
			}
			else
				checkDataLenFunc(kReliable);
		}
		return hasDataLeftThisRound;
	}

private:
	static const size_t kMaxMSS = 1472;
	static const size_t kMaxFrgCnt = 128;

	static const size_t kPktTypeLen = sizeof(int8_t);
	static const size_t kSnLen = sizeof(int32_t);
	static const size_t kFrgCntLen = sizeof(int8_t);
	static const size_t kFrgLen = sizeof(int8_t);
	static const size_t kDataLen = sizeof(int16_t);

	static const size_t kReliableHeaderLen = kPktTypeLen + kSnLen + kDataLen;
	static const size_t kReliableDataLenLimit = kMaxMSS - kReliableHeaderLen;
	
	static const size_t kUnreliableHeaderLen = kPktTypeLen + kSnLen + kFrgCntLen + kFrgLen + kDataLen;
	static const size_t kUnreliableDataLenLimit = kMaxMSS - kUnreliableHeaderLen;

	RecvFuncion rcvFunc_;
	UserOutputFunction userOutputFunc_;
	std::deque<std::string> outputPktDeque_;
	std::unordered_map<int, std::string> inputFrgMap_;
	int32_t nextSndSn_;
	int32_t nextRcvSn_;
	bool isThisRoundFinished_;
	bool on_;
	size_t mss_;
};




class KcpSession
{
public:
	KcpSession(const RoleTypeE role,
		const UserOutputFunction& userOutputFunc,
		const UserInputFunction& userInputFunc,
		const CurrentTimestampMsFunction& currentTimestampMsFunc)
		:
		role_(role),
		conv_(0),
		userInputFunc_(userInputFunc),
		curTsMsFunc_(currentTimestampMsFunc),
		kcp_(nullptr),
		curConnState_(kConnecting),
		rdc_(userOutputFunc, std::bind(&KcpSession::DoRecv, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)),
		nextUpdateTs_(0),
		hasDataLeft_(false),
		sndWnd_(128),
		rcvWnd_(128),
		waitSndCntLimit_(4 * sndWnd_),
		nodelay_(1),
		interval_(10),
		fastresend_(1),
		nocwnd_(1),
		streamMode_(0),
		mtu_(548),
		rx_minrto_(10)
	{
		if (IsClient() && !IsConnected())
			for (int i = 6; i > 0; --i)
				SendSyn();
	}

	/// ------ basic APIs --------

	// for Application-level Congestion Control
	bool CheckCanSend() const
	{
		return (kcp_ ? ikcp_waitsnd(kcp_) < waitSndCntLimit_ : true)
			&& (static_cast<int>(pendingSndDataDeque_.size()) < waitSndCntLimit_);
	}

	// returns below zero for error
	int Send(const void* data, int len, TransmitModeE transmitMode = kReliable)
	{ return SendImpl(data, len, transmitMode); }

	// update then returns next update timestamp in ms or returns below zero for error
	int64_t Update() { return UpdateImpl(); }

	// returns Is-Any-Data-Left, len below zero for error
	bool Recv(Buf* userBuf, int& len) { return RecvImpl(userBuf, len); }

	/// ------ advanced APIs --------

	bool IsServer() const { return role_ == kSrv; }
	bool IsClient() const { return role_ == kCli; }

	ikcpcb* GetKcpInstance() const { return kcp_; }

	bool IsConnected() const { return curConnState_ == kConnected; }

	// callback on :
	// - client role resetting state(when server role restart, client role will switch to resetting state
	//				and bring pending send data back to Application-level)
	// - cli/srv role connected state
	void setConnectionCallback(KcpSessionConnectionCallback cb) { connectionCallback_ = std::move(cb); }

	// should set before Send()
	void SetConfig(const int mtu = 576, const int sndWnd = 128, const int rcvWnd = 128,
		const int waitSndCntLimit = 512, const int nodelay = 1, const int interval = 10, const int fastresend = 1,
		const int nocwnd = 1, const int streamMode = 0, const int rx_minrto = 10)
	{
		assert(waitSndCntLimit > sndWnd);
		rdc_.SetMTU(mtu);
		sndWnd_ = sndWnd; rcvWnd_ = rcvWnd; waitSndCntLimit_ = waitSndCntLimit;
		nodelay_ = nodelay; interval_ = interval; fastresend_ = fastresend;
		nocwnd_ = nocwnd; streamMode_ = streamMode; rx_minrto_ = rx_minrto;
	}

	~KcpSession() { if (kcp_) ikcp_release(kcp_); }

private:

	int64_t UpdateImpl()
	{
		if (curConnState_ == kConnecting && IsClient())
			SendSyn();

		IUINT32 curTimestamp = static_cast<IUINT32>(curTsMsFunc_());
		if (kcp_ && IsConnected())
		{
			rdc_.Switch(ikcp_rdc_check(kcp_) == 1 ? true : false);
			int result = FlushSndQueueBeforeConned();
			if (result < 0)
				return result;
			if (curTimestamp >= nextUpdateTs_)
			{
				ikcp_update(kcp_, curTimestamp);
				nextUpdateTs_ = ikcp_check(kcp_, curTimestamp);
			}
			return static_cast<int64_t>(nextUpdateTs_);
		}
		else // not yet connected
			return static_cast<int64_t>(curTimestamp) + interval_;
	}

	int SendImpl(const void* data, int len, TransmitModeE transmitMode = kReliable)
	{
		assert(data != nullptr);
		assert(len > 0);
		assert(transmitMode == kReliable || transmitMode == kUnreliable);

		if (transmitMode == kUnreliable)
		{
			outputBuf_.append(data, len);
			int error = OutputAfterCheckingRdc(static_cast<PktTypeE>(kUnreliable));
			if (error)
				return error;
		}
		else if (transmitMode == kReliable)
		{
			if (!IsConnected() && IsClient())
			{
				pendingSndDataDeque_.emplace_back(std::string(static_cast<const char*>(data), len));
			}
			else if (IsConnected())
			{
				int result = FlushSndQueueBeforeConned();
				if (result < 0)
					return result;
				result = ikcp_send(kcp_, static_cast<const char*>(data), len);
				if (result < 0)
					return result; // ikcp_send err
				else
					ikcp_update(kcp_, static_cast<IUINT32>(curTsMsFunc_()));
			}
		}
		return 0;
	}

	bool RecvImpl(Buf* userBuf, int& len)
	{
		if (hasDataLeft_)
		{
			assert(inputBuf_.readableBytes() == 0);
			if (!IsConnected())
				return false;
			len = KcpRecv(userBuf); // if err, -1, -2, -3
			hasDataLeft_ = len > 0;
			return hasDataLeft_;
		}
		else
		{
			if (rdc_.IsThisRoundFinished())
			{
				const UserInputData& rawRecvdata = userInputFunc_();
				if (rawRecvdata.len_ < 0)
				{
					len = -10;
					return false;
				}
				else if (rawRecvdata.len_ > 0)
					inputBuf_.append(rawRecvdata.data_, rawRecvdata.len_);
			}
			if (!rdc_.Input(userBuf, len, &inputBuf_))
				hasDataLeft_ = true;
			return true;
		}
	}

	int FlushSndQueueBeforeConned()
	{
		assert(kcp_ && IsConnected());
		if (pendingSndDataDeque_.size() > 0)
		{
			for (auto it = pendingSndDataDeque_.begin(); it != pendingSndDataDeque_.end(); ++it)
			{
				int sendRet = ikcp_send(kcp_, it->c_str(), static_cast<int>(it->size()));
				if (sendRet < 0)
					return sendRet; // ikcp_send err
				ikcp_update(kcp_, static_cast<IUINT32>(curTsMsFunc_()));
			}
			pendingSndDataDeque_.clear();
		}
		return 0;
	}

	void CopyKcpDataToSndQ()
	{
		assert(kcp_);
		IKCPSEG *seg;
		struct IQUEUEHEAD *p;
		for (p = kcp_->snd_buf.next; p != &kcp_->snd_buf; p = p->next)
		{
			seg = iqueue_entry(p, IKCPSEG, node);
			pendingSndDataDeque_.emplace_back(std::string(seg->data, seg->len));
		}
		for (p = kcp_->snd_queue.next; p != &kcp_->snd_queue; p = p->next)
		{
			seg = iqueue_entry(p, IKCPSEG, node);
			pendingSndDataDeque_.emplace_back(std::string(seg->data, seg->len));
		}
	}

	void DoRecv(Buf* userBuf, int& len, int readableLen, PktTypeE pktType)
	{
		if (pktType == static_cast<PktTypeE>(kUnreliable))
		{
			userBuf->append(inputBuf_.peek(), readableLen);
			len = readableLen;
		}
		else if (pktType == kSyn)
		{
			assert(IsServer());
			if (!IsConnected())
			{
				SetConnState(kConnected);
				InitKcp(GetNewConv());
			}
			SendAckAndConv();
			len = 0;
		}
		else if (pktType == kAck)
		{
			assert(IsClient());
			int32_t rcvConv = inputBuf_.readInt32();
			readableLen -= 4;

			if (curConnState_ == kConnecting)
			{
				InitKcp(rcvConv);
				SetConnState(kConnected);
			}
			len = 0;
		}
		else if (pktType == kRst)
		{
			assert(IsClient());
			if (IsConnected())
			{
				CopyKcpDataToSndQ();
				SetConnState(kResetting);
			}
			len = 0;
		}
		else if (pktType == kPsh)
		{
			if (IsConnected())
			{
				int result = ikcp_input(kcp_, inputBuf_.peek(), readableLen);
				if (result == 0)
				{
					ikcp_update(kcp_, static_cast<IUINT32>(curTsMsFunc_()));
					len = 0;
				}
				else // if (result < 0)
					len = result - 3; // ikcp_input err, -4, -5, -6
			}
			else  // pktType == kPsh, but kcp not connected
			{
				if (IsServer())
					SendRst();
				len = 0;
			}
		}
		else
		{
			len = -7; // pktType err
		}
		inputBuf_.retrieve(readableLen);
	}

	void SendRst()
	{
		assert(IsServer());
		OutputAfterCheckingRdc(kRst);
	}

	void SendSyn()
	{
		assert(IsClient());
		OutputAfterCheckingRdc(kSyn);
	}

	void SendAckAndConv()
	{
		assert(IsServer());
		outputBuf_.appendInt32(conv_);
		OutputAfterCheckingRdc(kAck);
	}

	void InitKcp(const IUINT32 conv)
	{
		conv_ = conv;
		kcp_ = ikcp_create(conv, this);
		ikcp_wndsize(kcp_, sndWnd_, rcvWnd_);
		ikcp_nodelay(kcp_, nodelay_, interval_, fastresend_, nocwnd_);
		ikcp_setmtu(kcp_, mtu_);
		kcp_->stream = streamMode_;
		kcp_->rx_minrto = rx_minrto_;
		kcp_->output = KcpSession::KcpPshOutputFuncRaw;
	}

	IUINT32 GetNewConv()
	{
		assert(IsServer());
		static IUINT32 newConv = 666;
		return newConv++;
	}

	void SetConnState(const ConnectionStateE s)
	{
		ConnectionStateE lastState = curConnState_;
		curConnState_ = s;
		if (connectionCallback_ && lastState != s)
		{
			if (s == kConnected)
				connectionCallback_(nullptr);
			else if (s == kResetting)
				connectionCallback_(&pendingSndDataDeque_);
		}
	}

	int KcpRecv(Buf* userBuf)
	{
		assert(kcp_); assert(userBuf);
		int msgLen = ikcp_peeksize(kcp_);
		if (msgLen <= 0)
			return 0;
		userBuf->ensureWritableBytes(msgLen);
		ikcp_recv(kcp_, userBuf->beginWrite(), msgLen);
		userBuf->hasWritten(msgLen); // cause the ret of ikcp_recv() equal to ikcp_peeksize()
		return msgLen;
	}

	static int KcpPshOutputFuncRaw(const char* data, int len, IKCPCB* kcp, void* user)
	{
		(void)kcp;
		auto thisPtr = reinterpret_cast<KcpSession *>(user);
		thisPtr->outputBuf_.append(data, len);
		return thisPtr->OutputAfterCheckingRdc(kPsh);
	}

	int OutputAfterCheckingRdc(PktTypeE pktType) { return rdc_.Output(&outputBuf_, pktType); }

private:
	ikcpcb* kcp_;
	UserInputFunction userInputFunc_;
	ConnectionStateE curConnState_;
	Buf outputBuf_;
	Buf inputBuf_;
	CurrentTimestampMsFunction curTsMsFunc_;
	IUINT32 conv_;
	RoleTypeE role_;
	std::deque<std::string> pendingSndDataDeque_;
	Rdc rdc_;
	IUINT32 nextUpdateTs_;
	KcpSessionConnectionCallback connectionCallback_;
	bool hasDataLeft_;

private:
	// kcp config...
	int sndWnd_;
	int rcvWnd_;
	int waitSndCntLimit_;
	int nodelay_;
	int interval_;
	int fastresend_;
	int nocwnd_;
	int streamMode_;
	int mtu_;
	int rx_minrto_;
};

}