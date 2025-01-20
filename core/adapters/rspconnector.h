/*
Copyright 2020-2022 Vector 35 Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <array>
#include "binaryninjaapi.h"
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <unistd.h>
#endif
#include <cstring>
#include "socket.h"

namespace BinaryNinjaDebugger
{
	struct RspData
	{
		/* BUFFER_MAX/GDB_BUF_MAX - https://www.embecosm.com/appnotes/ean4/embecosm-howto-rsp-server-ean4-issue-2.pdf */
		/* i decided to double the max size of this as this is the 'GDB_BUF_MAX', but the rsp can be used by lldb as well */
		/* lldb's target.xml is way larger than gdbs and uses way more than 0x4000 bytes*/
		/* it is sometimes large than 0x8000 bytes as well, so enlarging it again */
		/* TODO: we will need a better approach to deal with the size later */

		static constexpr std::uint64_t BUFFER_MAX = (32 * 1024);

		struct RspIterator
		{
			using iterator_category = std::forward_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = std::uint8_t;
			using pointer           = value_type*;
			using reference         = value_type&;

			RspIterator(pointer ptr) : m_pointer(ptr) {}

			virtual reference operator*() const { return *m_pointer; }
			pointer operator->() { return m_pointer; }
			RspIterator& operator++() { m_pointer++; return *this; }
			RspIterator operator++(int) { RspIterator tmp = *this; ++(*this); return tmp; }
			friend bool operator== (const RspIterator& a, const RspIterator& b) { return a.m_pointer == b.m_pointer; };
			friend bool operator!= (const RspIterator& a, const RspIterator& b) { return a.m_pointer != b.m_pointer; };

		protected:
			pointer m_pointer;
		};

		struct ReverseRspIterator : public RspIterator
		{
			ReverseRspIterator(pointer ptr) : RspIterator(ptr) {}
			RspIterator& operator--() { m_pointer--; return *this; }
			RspIterator operator--(int) { RspIterator tmp = *this; --(*this); return tmp; }
		};

		struct ConstRspIterator : public RspIterator
		{
			ConstRspIterator(pointer ptr) : RspIterator(ptr) {}
			reference operator*() const override { return *m_pointer; }
		};

		RspIterator begin() const { return RspIterator((std::uint8_t*)m_data.GetDataAt(0)); }
		RspIterator end() const { return RspIterator((std::uint8_t*)m_data.GetDataAt(m_data.GetLength())); }
		ReverseRspIterator rbegin() const { return ReverseRspIterator((std::uint8_t*)m_data.GetDataAt(m_data.GetLength())); }
		ReverseRspIterator rend() const { return ReverseRspIterator((std::uint8_t*)m_data.GetDataAt(0)); }
		ConstRspIterator cbegin() const { return ConstRspIterator((std::uint8_t*)m_data.GetDataAt(0)); }
		ConstRspIterator cend() const { return ConstRspIterator((std::uint8_t*)m_data.GetDataAt(m_data.GetLength())); }

		RspData() {}

		template <typename... Args>
		explicit RspData(const std::string& string, Args... args)
		{
			std::string content = fmt::format(string.c_str(), args...);
			m_data = BinaryNinja::DataBuffer(content.data(), content.size());
		}

		explicit


		RspData(const std::string& str)
		{
			m_data = BinaryNinja::DataBuffer(str.data(), str.size());
		}

		RspData(void* data, std::size_t size)
		{
			m_data = BinaryNinja::DataBuffer(data, size);
		}

		RspData(const char* data, std::size_t size)
		{
			m_data = BinaryNinja::DataBuffer(data, size);
		}

		~RspData()
		{
		}

		[[nodiscard]] std::string AsString() const
		{
			if (m_data.GetLength())
				return std::string((char*)m_data.GetData(), m_data.GetLength());
			else
				return std::string{};
		}

		BinaryNinja::DataBuffer m_data;

		uint8_t& operator[](size_t offset);
		const uint8_t& operator[](size_t offset) const;
	};


	class RspConnector
	{
		std::recursive_mutex m_socketLock;

		Socket* m_socket{};
		bool m_acksEnabled{true};
		std::vector<std::string> m_serverCapabilities{};
		int m_maxPacketLength{0xfff};

	public:
		RspConnector() = default;
		RspConnector(Socket* socket);
		~RspConnector();

		static RspData BinaryDecode(const RspData& data);
		static RspData DecodeRLE(const RspData& data);
		static std::unordered_map<std::string, std::uint64_t> PacketToUnorderedMap(const RspData& data);
		static std::vector<std::string> Split(const std::string& string, const std::string& regex);

		static uint64_t SwapEndianness(uint64_t value, size_t len)
		{
			switch (len)
			{
			case 1:
				return SwapEndianness((uint8_t)value);
			case 2:
				return SwapEndianness((uint16_t)value);
			case 4:
				return SwapEndianness((uint32_t)value);
			case 8:
				return SwapEndianness(value);
			default:
				return value;
			}
		}

		template <typename Ty>
		static Ty SwapEndianness(Ty value) {
			union {
				Ty m_val;
				std::array<std::uint8_t, sizeof(Ty)> m_raw;
			} source{value}, dest{};
			std::reverse_copy(source.m_raw.begin(), source.m_raw.end(), dest.m_raw.begin());
			return dest.m_val;
		}


		void EnableAcks();
		void DisableAcks();

		char ExpectAck();
		void SendAck() const;

		void NegotiateCapabilities(const std::vector<std::string>& capabilities);

		void SendRaw(const RspData& data) const;
		void SendPayload(const RspData& data) const;

		RspData ReceiveRspData() const;
		RspData TransmitAndReceive(const RspData& data, const std::string& expect = "ack_then_reply",
								   std::function<void(const RspData& data)> asyncPacketHandler = nullptr);
		int32_t HostFileIO(const RspData& data, RspData& output, int32_t& error);

		std::string GetXml(const std::string& name);
	};
};
