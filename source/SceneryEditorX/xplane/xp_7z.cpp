/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * xp_7z.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "xp_7z.h"
#include <algorithm>
#include <array>
#include <cstring>

// The decoder runs a few hundred million bit decodes per Global Scenery tile: keep it optimized in MSVC debug builds
// too, or loading the terrain around an airport takes tens of seconds there.
#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("gt", on)
#endif

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		bool Fail(std::string *error, const std::string &message)
		{
			if (error)
				*error = message;
			return false;
		}

		// -------------------------------------------------------
		// LZMA (lzma-specification.txt): a range coder over adaptive binary probabilities

		using Prob = uint16_t;
		constexpr uint32_t NUM_BIT_MODEL_TOTAL_BITS = 11;
		constexpr uint32_t BIT_MODEL_TOTAL = 1u << NUM_BIT_MODEL_TOTAL_BITS;
		constexpr uint32_t NUM_MOVE_BITS = 5;
		constexpr uint32_t TOP_VALUE = 1u << 24;
		constexpr Prob PROB_INIT = BIT_MODEL_TOTAL / 2;

		constexpr uint32_t NUM_STATES = 12;
		constexpr uint32_t NUM_POS_BITS_MAX = 4;
		constexpr uint32_t NUM_LEN_TO_POS_STATES = 4;
		constexpr uint32_t NUM_ALIGN_BITS = 4;
		constexpr uint32_t START_POS_MODEL_INDEX = 4;
		constexpr uint32_t END_POS_MODEL_INDEX = 14;
		constexpr uint32_t NUM_FULL_DISTANCES = 1u << (END_POS_MODEL_INDEX >> 1);
		constexpr uint32_t MATCH_MIN_LEN = 2;

		class RangeDecoder
		{
		public:
			/* @brief Starts on a stream: a zero byte, then the first four code bytes. */
			bool Init(const uint8_t *data, const size_t size)
			{
				m_Data = data;
				m_Size = size;
				m_Position = 0;
				m_Range = 0xFFFFFFFFu;
				m_Code = 0;
				m_Corrupted = false;
				const uint8_t first = NextByte();
				for (int i = 0; i < 4; ++i)
					m_Code = (m_Code << 8) | NextByte();
				return first == 0 && m_Code != m_Range && !Overrun();
			}

			uint32_t DecodeDirectBits(uint32_t numBits)
			{
				uint32_t result = 0;
				do
				{
					m_Range >>= 1;
					m_Code -= m_Range;
					const uint32_t t = 0u - (m_Code >> 31);
					m_Code += m_Range & t;
					if (m_Code == m_Range)
						m_Corrupted = true;
					Normalize();
					result = (result << 1) + (t + 1);
				} while (--numBits);
				return result;
			}

			uint32_t DecodeBit(Prob *prob)
			{
				uint32_t value = *prob;
				const uint32_t bound = (m_Range >> NUM_BIT_MODEL_TOTAL_BITS) * value;
				uint32_t symbol;
				if (m_Code < bound)
				{
					value += (BIT_MODEL_TOTAL - value) >> NUM_MOVE_BITS;
					m_Range = bound;
					symbol = 0;
				}
				else
				{
					value -= value >> NUM_MOVE_BITS;
					m_Code -= bound;
					m_Range -= bound;
					symbol = 1;
				}
				*prob = static_cast<Prob>(value);
				Normalize();
				return symbol;
			}

			[[nodiscard]] bool Overrun() const { return m_Position > m_Size; }
			[[nodiscard]] bool Corrupted() const { return m_Corrupted; }

		private:
			uint8_t NextByte()
			{
				// Past the end reads zeros and remembers it (a valid stream never needs more than it has)
				if (m_Position < m_Size)
					return m_Data[m_Position++];
				++m_Position;
				return 0;
			}

			void Normalize()
			{
				if (m_Range < TOP_VALUE)
				{
					m_Range <<= 8;
					m_Code = (m_Code << 8) | NextByte();
				}
			}

			const uint8_t *m_Data = nullptr;
			size_t m_Size = 0;
			size_t m_Position = 0;
			uint32_t m_Range = 0;
			uint32_t m_Code = 0;
			bool m_Corrupted = false;
		};

		template<uint32_t NumBits>
		struct BitTree
		{
			std::array<Prob, 1u << NumBits> probs{};

			void Init() { probs.fill(PROB_INIT); }

			uint32_t Decode(RangeDecoder &rc)
			{
				uint32_t m = 1;
				for (uint32_t i = 0; i < NumBits; ++i)
					m = (m << 1) + rc.DecodeBit(&probs[m]);
				return m - (1u << NumBits);
			}
		};

		uint32_t BitTreeReverseDecode(Prob *probs, const uint32_t numBits, RangeDecoder &rc)
		{
			uint32_t m = 1, symbol = 0;
			for (uint32_t i = 0; i < numBits; ++i)
			{
				const uint32_t bit = rc.DecodeBit(&probs[m]);
				m = (m << 1) + bit;
				symbol |= bit << i;
			}
			return symbol;
		}

		struct LenDecoder
		{
			Prob choice = PROB_INIT;
			Prob choice2 = PROB_INIT;
			std::array<BitTree<3>, 1u << NUM_POS_BITS_MAX> low;
			std::array<BitTree<3>, 1u << NUM_POS_BITS_MAX> mid;
			BitTree<8> high;

			void Init()
			{
				choice = choice2 = PROB_INIT;
				high.Init();
				for (auto &tree : low)
					tree.Init();
				for (auto &tree : mid)
					tree.Init();
			}

			uint32_t Decode(RangeDecoder &rc, const uint32_t posState)
			{
				if (rc.DecodeBit(&choice) == 0)
					return low[posState].Decode(rc);
				if (rc.DecodeBit(&choice2) == 0)
					return 8 + mid[posState].Decode(rc);
				return 16 + high.Decode(rc);
			}
		};

		/* @brief Probabilities and the state machine of one LZMA stream (LZMA2 keeps them across chunks). */
		class LzmaDecoder
		{
		public:
			bool SetProperties(uint32_t d)
			{
				if (d >= 9 * 5 * 5)
					return false;
				m_Lc = d % 9;
				d /= 9;
				m_Lp = d % 5;
				m_Pb = d / 5;
				m_Literals.assign(static_cast<size_t>(0x300) << (m_Lc + m_Lp), PROB_INIT);
				return true;
			}

			[[nodiscard]] uint32_t GetLiteralBits() const { return m_Lc + m_Lp; }

			void ResetProbabilities()
			{
				std::fill(m_Literals.begin(), m_Literals.end(), PROB_INIT);
				for (auto &tree : m_PosSlot)
					tree.Init();
				m_Align.Init();
				std::fill(std::begin(m_PosDecoders), std::end(m_PosDecoders), PROB_INIT);
				std::fill(std::begin(m_IsMatch), std::end(m_IsMatch), PROB_INIT);
				std::fill(std::begin(m_IsRep), std::end(m_IsRep), PROB_INIT);
				std::fill(std::begin(m_IsRepG0), std::end(m_IsRepG0), PROB_INIT);
				std::fill(std::begin(m_IsRepG1), std::end(m_IsRepG1), PROB_INIT);
				std::fill(std::begin(m_IsRepG2), std::end(m_IsRepG2), PROB_INIT);
				std::fill(std::begin(m_IsRep0Long), std::end(m_IsRep0Long), PROB_INIT);
				m_Len.Init();
				m_RepLen.Init();
			}

			void ResetState()
			{
				m_State = 0;
				m_Rep0 = m_Rep1 = m_Rep2 = m_Rep3 = 0;
			}

			/**
			 * @brief Decodes into out[position, end). Matches may reach back to dictionaryStart (where the last dictionary
			 * reset was), not before it; position bits count from there too.
			 * @return False on corrupt data (an end marker before end counts as corrupt: 7-zip records the exact size).
			 */
			bool Decode(RangeDecoder &rc, uint8_t *out, size_t &position, const size_t end, const size_t dictionaryStart, std::string *error)
			{
				const uint32_t pbMask = (1u << m_Pb) - 1;
				const uint32_t lpMask = (1u << m_Lp) - 1;
				size_t pos = position;
				while (pos < end)
				{
					const size_t dictPos = pos - dictionaryStart;
					const uint32_t posState = static_cast<uint32_t>(dictPos) & pbMask;
					if (rc.DecodeBit(&m_IsMatch[(m_State << NUM_POS_BITS_MAX) + posState]) == 0)
					{
						// Literal, coded against the byte a match would have copied after a match
						const uint32_t previous = dictPos > 0 ? out[pos - 1] : 0;
						const uint32_t litState = ((static_cast<uint32_t>(dictPos) & lpMask) << m_Lc) + (previous >> (8 - m_Lc));
						Prob *probs = &m_Literals[static_cast<size_t>(0x300) * litState];
						uint32_t symbol = 1;
						if (m_State >= 7)
						{
							uint32_t matchByte = out[pos - m_Rep0 - 1];
							do
							{
								const uint32_t matchBit = (matchByte >> 7) & 1;
								matchByte <<= 1;
								const uint32_t bit = rc.DecodeBit(&probs[((1 + matchBit) << 8) + symbol]);
								symbol = (symbol << 1) | bit;
								if (matchBit != bit)
									break;
							} while (symbol < 0x100);
						}
						while (symbol < 0x100)
							symbol = (symbol << 1) | rc.DecodeBit(&probs[symbol]);
						out[pos++] = static_cast<uint8_t>(symbol - 0x100);
						m_State = m_State < 4 ? 0 : (m_State < 10 ? m_State - 3 : m_State - 6);
						continue;
					}

					uint32_t length;
					if (rc.DecodeBit(&m_IsRep[m_State]) != 0)
					{
						if (dictPos == 0)
							return Fail(error, "LZMA data repeats a match before any output");
						if (rc.DecodeBit(&m_IsRepG0[m_State]) == 0)
						{
							if (rc.DecodeBit(&m_IsRep0Long[(m_State << NUM_POS_BITS_MAX) + posState]) == 0)
							{
								// Short rep: one byte from the last distance
								m_State = m_State < 7 ? 9 : 11;
								out[pos] = out[pos - m_Rep0 - 1];
								++pos;
								continue;
							}
						}
						else
						{
							uint32_t distance;
							if (rc.DecodeBit(&m_IsRepG1[m_State]) == 0)
								distance = m_Rep1;
							else
							{
								if (rc.DecodeBit(&m_IsRepG2[m_State]) == 0)
									distance = m_Rep2;
								else
								{
									distance = m_Rep3;
									m_Rep3 = m_Rep2;
								}
								m_Rep2 = m_Rep1;
							}
							m_Rep1 = m_Rep0;
							m_Rep0 = distance;
						}
						length = m_RepLen.Decode(rc, posState);
						m_State = m_State < 7 ? 8 : 11;
					}
					else
					{
						m_Rep3 = m_Rep2;
						m_Rep2 = m_Rep1;
						m_Rep1 = m_Rep0;
						length = m_Len.Decode(rc, posState);
						m_State = m_State < 7 ? 7 : 10;
						m_Rep0 = DecodeDistance(rc, length);
						if (m_Rep0 == 0xFFFFFFFFu)
						{
							position = pos;
							return Fail(error, "LZMA end marker before the recorded size");
						}
						if (m_Rep0 >= dictPos)
							return Fail(error, "LZMA match reaches before the start of the data");
					}

					length += MATCH_MIN_LEN;
					if (length > end - pos)
						return Fail(error, "LZMA match runs past the recorded size");
					const uint8_t *from = out + pos - m_Rep0 - 1;
					uint8_t *to = out + pos;
					for (uint32_t i = 0; i < length; ++i)
						to[i] = from[i];		// Byte by byte: a match may overlap its own output
					pos += length;
				}
				position = pos;
				if (rc.Overrun())
					return Fail(error, "LZMA data ends early");
				if (rc.Corrupted())
					return Fail(error, "Corrupt LZMA data");
				return true;
			}

		private:
			uint32_t DecodeDistance(RangeDecoder &rc, const uint32_t length)
			{
				const uint32_t lenState = std::min(length, NUM_LEN_TO_POS_STATES - 1);
				const uint32_t posSlot = m_PosSlot[lenState].Decode(rc);
				if (posSlot < START_POS_MODEL_INDEX)
					return posSlot;
				const uint32_t numDirectBits = (posSlot >> 1) - 1;
				uint32_t distance = (2 | (posSlot & 1)) << numDirectBits;
				if (posSlot < END_POS_MODEL_INDEX)
					distance += BitTreeReverseDecode(m_PosDecoders + distance - posSlot, numDirectBits, rc);
				else
				{
					distance += rc.DecodeDirectBits(numDirectBits - NUM_ALIGN_BITS) << NUM_ALIGN_BITS;
					distance += BitTreeReverseDecode(m_Align.probs.data(), NUM_ALIGN_BITS, rc);
				}
				return distance;
			}

			uint32_t m_Lc = 3, m_Lp = 0, m_Pb = 2;
			std::vector<Prob> m_Literals;
			std::array<BitTree<6>, NUM_LEN_TO_POS_STATES> m_PosSlot;
			BitTree<NUM_ALIGN_BITS> m_Align;
			Prob m_PosDecoders[1 + NUM_FULL_DISTANCES - END_POS_MODEL_INDEX] = {};
			Prob m_IsMatch[NUM_STATES << NUM_POS_BITS_MAX] = {};
			Prob m_IsRep[NUM_STATES] = {};
			Prob m_IsRepG0[NUM_STATES] = {};
			Prob m_IsRepG1[NUM_STATES] = {};
			Prob m_IsRepG2[NUM_STATES] = {};
			Prob m_IsRep0Long[NUM_STATES << NUM_POS_BITS_MAX] = {};
			LenDecoder m_Len;
			LenDecoder m_RepLen;
			uint32_t m_State = 0;
			uint32_t m_Rep0 = 0, m_Rep1 = 0, m_Rep2 = 0, m_Rep3 = 0;
		};

		// -------------------------------------------------------
		// 7-zip container (7zFormat.txt): property ids and the variable-length numbers headers are made of

		enum : uint8_t
		{
			ID_END = 0x00,
			ID_HEADER = 0x01,
			ID_ARCHIVE_PROPERTIES = 0x02,
			ID_ADDITIONAL_STREAMS_INFO = 0x03,
			ID_MAIN_STREAMS_INFO = 0x04,
			ID_FILES_INFO = 0x05,
			ID_PACK_INFO = 0x06,
			ID_UNPACK_INFO = 0x07,
			ID_SUBSTREAMS_INFO = 0x08,
			ID_SIZE = 0x09,
			ID_CRC = 0x0A,
			ID_FOLDER = 0x0B,
			ID_CODERS_UNPACK_SIZE = 0x0C,
			ID_NUM_UNPACK_STREAM = 0x0D,
			ID_ENCODED_HEADER = 0x17
		};

		constexpr uint64_t MAX_UNPACK_SIZE = 1ull << 31;		// 2 GB: far above any DSF tile

		class HeaderReader
		{
		public:
			HeaderReader(const uint8_t *data, const size_t size) : m_Data(data), m_Size(size) {}

			uint8_t Byte()
			{
				if (m_Position < m_Size)
					return m_Data[m_Position++];
				m_Bad = true;
				return 0;
			}

			uint32_t U32()
			{
				uint32_t value = 0;
				for (int i = 0; i < 4; ++i)
					value |= static_cast<uint32_t>(Byte()) << (8 * i);
				return value;
			}

			/* @brief 7-zip's UINT64: the first byte's leading one bits count the extra bytes, its rest are the high bits. */
			uint64_t Number()
			{
				const uint8_t first = Byte();
				uint8_t mask = 0x80;
				uint64_t value = 0;
				for (int i = 0; i < 8; ++i)
				{
					if ((first & mask) == 0)
					{
						const uint64_t high = first & (mask - 1u);
						value |= high << (8 * i);
						return value;
					}
					value |= static_cast<uint64_t>(Byte()) << (8 * i);
					mask >>= 1;
				}
				return value;
			}

			void Skip(const uint64_t count)
			{
				if (count > m_Size - std::min(m_Position, m_Size))
				{
					m_Bad = true;
					m_Position = m_Size;
					return;
				}
				m_Position += static_cast<size_t>(count);
			}

			std::vector<uint8_t> Bytes(const uint64_t count)
			{
				std::vector<uint8_t> bytes;
				if (count > m_Size - std::min(m_Position, m_Size))
				{
					m_Bad = true;
					m_Position = m_Size;
					return bytes;
				}
				bytes.assign(m_Data + m_Position, m_Data + m_Position + count);
				m_Position += static_cast<size_t>(count);
				return bytes;
			}

			[[nodiscard]] bool Bad() const { return m_Bad; }

		private:
			const uint8_t *m_Data;
			size_t m_Size;
			size_t m_Position = 0;
			bool m_Bad = false;
		};

		struct Coder
		{
			std::vector<uint8_t> id;
			std::vector<uint8_t> properties;
			uint64_t inStreams = 1;
			uint64_t outStreams = 1;
		};

		struct Folder
		{
			std::vector<Coder> coders;
			uint64_t bindPairs = 0;
			uint64_t packedStreams = 1;
			std::vector<uint64_t> unpackSizes;		// One per coder output
			bool hasCrc = false;
			uint32_t crc = 0;
			uint64_t unpackStreams = 1;			// Files in the folder (SubStreamsInfo)
			std::vector<uint64_t> streamSizes;	// Their sizes when more than one

			[[nodiscard]] uint64_t GetUnpackSize() const { return unpackSizes.empty() ? 0 : unpackSizes.back(); }
		};

		struct StreamsInfo
		{
			uint64_t packPosition = 0;
			std::vector<uint64_t> packSizes;
			std::vector<Folder> folders;
		};

		/* @brief A digest list: all-defined flag (else a bit field), then a CRC per defined item. */
		void ReadDigests(HeaderReader &reader, const uint64_t count, std::vector<std::pair<bool, uint32_t>> &out)
		{
			out.assign(static_cast<size_t>(std::min<uint64_t>(count, 1u << 20)), { false, 0u });
			const bool allDefined = reader.Byte() != 0;
			if (!allDefined)
			{
				uint8_t byte = 0, mask = 0;
				for (auto &entry : out)
				{
					if (mask == 0)
					{
						byte = reader.Byte();
						mask = 0x80;
					}
					entry.first = (byte & mask) != 0;
					mask >>= 1;
				}
			}
			else
			{
				for (auto &entry : out)
					entry.first = true;
			}
			for (auto &entry : out)
			{
				if (entry.first)
					entry.second = reader.U32();
			}
		}

		bool ReadFolder(HeaderReader &reader, Folder &folder, std::string *error)
		{
			const uint64_t coders = reader.Number();
			if (coders == 0 || coders > 32)
				return Fail(error, "7-zip folder with " + std::to_string(coders) + " coders");
			uint64_t totalIn = 0, totalOut = 0;
			for (uint64_t i = 0; i < coders; ++i)
			{
				Coder coder;
				const uint8_t flags = reader.Byte();
				if (flags & 0x80)
					return Fail(error, "7-zip alternative coder methods aren't supported");
				coder.id = reader.Bytes(flags & 0x0F);
				if (flags & 0x10)
				{
					coder.inStreams = reader.Number();
					coder.outStreams = reader.Number();
				}
				if (flags & 0x20)
					coder.properties = reader.Bytes(reader.Number());
				totalIn += coder.inStreams;
				totalOut += coder.outStreams;
				folder.coders.push_back(std::move(coder));
			}
			if (totalOut == 0 || totalOut > 64 || totalIn > 64)
				return Fail(error, "Corrupt 7-zip folder");
			folder.bindPairs = totalOut - 1;
			for (uint64_t i = 0; i < folder.bindPairs; ++i)
			{
				reader.Number();
				reader.Number();
			}
			if (totalIn < folder.bindPairs)
				return Fail(error, "Corrupt 7-zip folder bindings");
			folder.packedStreams = totalIn - folder.bindPairs;
			if (folder.packedStreams > 1)
			{
				for (uint64_t i = 0; i < folder.packedStreams; ++i)
					reader.Number();
			}
			return !reader.Bad() || Fail(error, "Truncated 7-zip folder");
		}

		bool ReadStreamsInfo(HeaderReader &reader, StreamsInfo &info, std::string *error)
		{
			for (;;)
			{
				const uint64_t type = reader.Number();
				if (reader.Bad())
					return Fail(error, "Truncated 7-zip header");
				if (type == ID_END)
					return true;

				if (type == ID_PACK_INFO)
				{
					info.packPosition = reader.Number();
					const uint64_t count = reader.Number();
					if (count > 1024)
						return Fail(error, "7-zip archive with " + std::to_string(count) + " packed streams");
					info.packSizes.assign(static_cast<size_t>(count), 0);
					for (;;)
					{
						const uint64_t id = reader.Number();
						if (id == ID_END || reader.Bad())
							break;
						if (id == ID_SIZE)
						{
							for (uint64_t &size : info.packSizes)
								size = reader.Number();
						}
						else if (id == ID_CRC)
						{
							std::vector<std::pair<bool, uint32_t>> digests;
							ReadDigests(reader, count, digests);
						}
						else
							return Fail(error, "Unexpected 7-zip pack info property " + std::to_string(id));
					}
				}
				else if (type == ID_UNPACK_INFO)
				{
					if (reader.Number() != ID_FOLDER)
						return Fail(error, "7-zip unpack info without folders");
					const uint64_t count = reader.Number();
					if (count > 1024)
						return Fail(error, "7-zip archive with " + std::to_string(count) + " folders");
					if (reader.Byte() != 0)
						return Fail(error, "7-zip external folder lists aren't supported");
					info.folders.assign(static_cast<size_t>(count), Folder{});
					for (Folder &folder : info.folders)
					{
						if (!ReadFolder(reader, folder, error))
							return false;
					}
					if (reader.Number() != ID_CODERS_UNPACK_SIZE)
						return Fail(error, "7-zip folders without unpack sizes");
					for (Folder &folder : info.folders)
					{
						uint64_t outputs = 0;
						for (const Coder &coder : folder.coders)
							outputs += coder.outStreams;
						for (uint64_t i = 0; i < outputs; ++i)
							folder.unpackSizes.push_back(reader.Number());
					}
					for (;;)
					{
						const uint64_t id = reader.Number();
						if (id == ID_END || reader.Bad())
							break;
						if (id == ID_CRC)
						{
							std::vector<std::pair<bool, uint32_t>> digests;
							ReadDigests(reader, info.folders.size(), digests);
							for (size_t i = 0; i < digests.size() && i < info.folders.size(); ++i)
							{
								info.folders[i].hasCrc = digests[i].first;
								info.folders[i].crc = digests[i].second;
							}
						}
						else
							return Fail(error, "Unexpected 7-zip unpack info property " + std::to_string(id));
					}
				}
				else if (type == ID_SUBSTREAMS_INFO)
				{
					for (;;)
					{
						const uint64_t id = reader.Number();
						if (id == ID_END || reader.Bad())
							break;
						if (id == ID_NUM_UNPACK_STREAM)
						{
							for (Folder &folder : info.folders)
								folder.unpackStreams = reader.Number();
						}
						else if (id == ID_SIZE)
						{
							for (Folder &folder : info.folders)
							{
								if (folder.unpackStreams == 0)
									continue;
								uint64_t sum = 0;
								for (uint64_t i = 1; i < folder.unpackStreams && !reader.Bad(); ++i)
								{
									const uint64_t size = reader.Number();
									folder.streamSizes.push_back(size);
									sum += size;
								}
								folder.streamSizes.push_back(folder.GetUnpackSize() >= sum ? folder.GetUnpackSize() - sum : 0);
							}
						}
						else if (id == ID_CRC)
						{
							uint64_t count = 0;
							for (const Folder &folder : info.folders)
								count += folder.unpackStreams == 1 && folder.hasCrc ? 0 : folder.unpackStreams;
							std::vector<std::pair<bool, uint32_t>> digests;
							ReadDigests(reader, count, digests);
						}
						else
							return Fail(error, "Unexpected 7-zip substreams property " + std::to_string(id));
					}
				}
				else
					return Fail(error, "Unexpected 7-zip header property " + std::to_string(type));
			}
		}

		/* @brief Unpacks folder 0 of a streams info whose packed data starts at packBase in the archive. */
		bool DecodeFirstFolder(const uint8_t *archive, const size_t archiveSize, const StreamsInfo &info, std::vector<uint8_t> &out, std::string *error)
		{
			if (info.folders.empty() || info.packSizes.empty())
				return Fail(error, "The 7-zip archive holds no data");
			const Folder &folder = info.folders.front();
			if (folder.coders.size() != 1 || folder.packedStreams != 1)
				return Fail(error, "7-zip filter chains (BCJ, encryption) aren't supported");

			const uint64_t packStart = 32 + info.packPosition;
			const uint64_t packSize = info.packSizes.front();
			if (packStart > archiveSize || packSize > archiveSize - packStart)
				return Fail(error, "The 7-zip archive is truncated");
			const uint64_t unpackSize = folder.GetUnpackSize();
			if (unpackSize > MAX_UNPACK_SIZE)
				return Fail(error, "The 7-zip archive unpacks to " + std::to_string(unpackSize) + " bytes");

			out.assign(static_cast<size_t>(unpackSize), 0);
			const uint8_t *packed = archive + packStart;
			const Coder &coder = folder.coders.front();
			static constexpr uint8_t LZMA_ID[] = { 0x03, 0x01, 0x01 };
			if (coder.id.size() == 3 && std::equal(coder.id.begin(), coder.id.end(), LZMA_ID))
			{
				if (!DecodeLzma(coder.properties.data(), coder.properties.size(), packed, static_cast<size_t>(packSize), out.data(), out.size(), error))
					return false;
			}
			else if (coder.id.size() == 1 && coder.id[0] == 0x21)
			{
				if (!DecodeLzma2(packed, static_cast<size_t>(packSize), out.data(), out.size(), error))
					return false;
			}
			else if (coder.id.size() == 1 && coder.id[0] == 0x00)
			{
				if (packSize < unpackSize)
					return Fail(error, "The 7-zip archive is truncated");
				std::memcpy(out.data(), packed, out.size());
			}
			else
			{
				std::string id;
				for (const uint8_t byte : coder.id)
				{
					static constexpr char HEX[] = "0123456789abcdef";
					id += HEX[byte >> 4];
					id += HEX[byte & 15];
				}
				return Fail(error, "Unsupported 7-zip compression method " + id);
			}

			if (folder.hasCrc && Crc32(out.data(), out.size()) != folder.crc)
				return Fail(error, "The 7-zip archive fails its CRC check");
			return true;
		}

		uint64_t ReadLe64(const uint8_t *p)
		{
			uint64_t value = 0;
			for (int i = 7; i >= 0; --i)
				value = (value << 8) | p[i];
			return value;
		}
	}

	// -------------------------------------------------------

	bool Is7zArchive(const uint8_t *data, const size_t size)
	{
		return data && size >= sizeof(SEVEN_ZIP_SIGNATURE) && std::memcmp(data, SEVEN_ZIP_SIGNATURE, sizeof(SEVEN_ZIP_SIGNATURE)) == 0;
	}

	uint32_t Crc32(const uint8_t *data, const size_t size, uint32_t crc)
	{
		static const std::array<uint32_t, 256> table = []
		{
			std::array<uint32_t, 256> t{};
			for (uint32_t i = 0; i < 256; ++i)
			{
				uint32_t c = i;
				for (int k = 0; k < 8; ++k)
					c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
				t[i] = c;
			}
			return t;
		}();
		crc = ~crc;
		for (size_t i = 0; i < size; ++i)
			crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
		return ~crc;
	}

	bool DecodeLzma(const uint8_t *properties, const size_t propertiesSize, const uint8_t *data, const size_t size, uint8_t *out, const size_t outSize, std::string *error)
	{
		if (!properties || propertiesSize < 5)
			return Fail(error, "LZMA properties missing");
		LzmaDecoder decoder;
		if (!decoder.SetProperties(properties[0]))
			return Fail(error, "Bad LZMA properties");
		decoder.ResetProbabilities();
		decoder.ResetState();
		RangeDecoder rc;
		if (!rc.Init(data, size))
			return Fail(error, "Corrupt LZMA stream start");
		size_t position = 0;
		return decoder.Decode(rc, out, position, outSize, 0, error);
	}

	bool DecodeLzma2(const uint8_t *data, const size_t size, uint8_t *out, const size_t outSize, std::string *error)
	{
		LzmaDecoder decoder;
		bool haveProperties = false;
		bool haveDictionary = false;
		size_t in = 0, position = 0, dictionaryStart = 0;
		for (;;)
		{
			if (in >= size)
				return Fail(error, "LZMA2 data ends early");
			const uint8_t control = data[in++];
			if (control == 0x00)
				break;

			if (control == 0x01 || control == 0x02)
			{
				// Stored chunk (0x01 also resets the dictionary)
				if (in + 2 > size)
					return Fail(error, "LZMA2 data ends early");
				const size_t chunk = ((static_cast<size_t>(data[in]) << 8) | data[in + 1]) + 1;
				in += 2;
				if (control == 0x01)
				{
					dictionaryStart = position;
					haveDictionary = true;
				}
				else if (!haveDictionary)
					return Fail(error, "LZMA2 data starts without a dictionary reset");
				if (chunk > size - in || chunk > outSize - position)
					return Fail(error, "LZMA2 stored chunk runs past the data");
				std::memcpy(out + position, data + in, chunk);
				in += chunk;
				position += chunk;
				continue;
			}
			if (control < 0x80)
				return Fail(error, "Bad LZMA2 chunk header");

			if (in + 4 > size)
				return Fail(error, "LZMA2 data ends early");
			const size_t unpacked = ((static_cast<size_t>(control & 0x1F) << 16) | (static_cast<size_t>(data[in]) << 8) | data[in + 1]) + 1;
			const size_t packed = ((static_cast<size_t>(data[in + 2]) << 8) | data[in + 3]) + 1;
			in += 4;
			const uint32_t reset = (control >> 5) & 3;	// 0 none, 1 state, 2 state + properties, 3 everything + dictionary
			if (reset == 3)
			{
				dictionaryStart = position;
				haveDictionary = true;
			}
			else if (!haveDictionary)
				return Fail(error, "LZMA2 data starts without a dictionary reset");
			if (reset >= 2)
			{
				if (in >= size || !decoder.SetProperties(data[in]) || decoder.GetLiteralBits() > 4)
					return Fail(error, "Bad LZMA2 properties");
				++in;
				haveProperties = true;
			}
			else if (!haveProperties)
				return Fail(error, "LZMA2 chunk without properties");
			if (reset >= 1)
			{
				decoder.ResetProbabilities();
				decoder.ResetState();
			}

			if (packed > size - in || unpacked > outSize - position)
				return Fail(error, "LZMA2 chunk runs past the data");
			RangeDecoder rc;
			if (!rc.Init(data + in, packed))
				return Fail(error, "Corrupt LZMA2 chunk");
			if (!decoder.Decode(rc, out, position, position + unpacked, dictionaryStart, error))
				return false;
			in += packed;
		}
		if (position != outSize)
			return Fail(error, "LZMA2 data is shorter than its recorded size");
		return true;
	}

	bool Extract7zFirstFile(const uint8_t *data, const size_t size, std::vector<uint8_t> &out, std::string *error)
	{
		out.clear();
		if (!Is7zArchive(data, size) || size < 32)
			return Fail(error, "Not a 7-zip archive");
		if (data[6] != 0)
			return Fail(error, "Unsupported 7-zip format version " + std::to_string(data[6]));

		const uint64_t nextOffset = ReadLe64(data + 12);
		const uint64_t nextSize = ReadLe64(data + 20);
		const uint32_t nextCrc = static_cast<uint32_t>(data[28]) | (static_cast<uint32_t>(data[29]) << 8) | (static_cast<uint32_t>(data[30]) << 16) | (static_cast<uint32_t>(data[31]) << 24);
		if (nextOffset > size - 32 || nextSize > size - 32 - nextOffset || nextSize == 0)
			return Fail(error, "The 7-zip archive is truncated (or a multi-volume part)");
		if (Crc32(data + 32 + nextOffset, static_cast<size_t>(nextSize)) != nextCrc)
			return Fail(error, "The 7-zip header fails its CRC check");

		std::vector<uint8_t> header(data + 32 + nextOffset, data + 32 + nextOffset + nextSize);
		for (int depth = 0; depth < 4; ++depth)
		{
			HeaderReader reader(header.data(), header.size());
			const uint64_t type = reader.Number();
			if (type == ID_ENCODED_HEADER)
			{
				StreamsInfo info;
				if (!ReadStreamsInfo(reader, info, error))
					return false;
				std::vector<uint8_t> decoded;
				if (!DecodeFirstFolder(data, size, info, decoded, error))
					return false;
				header = std::move(decoded);
				continue;
			}
			if (type != ID_HEADER)
				return Fail(error, "Unexpected 7-zip header type " + std::to_string(type));

			StreamsInfo main;
			for (;;)
			{
				const uint64_t id = reader.Number();
				if (reader.Bad())
					return Fail(error, "Truncated 7-zip header");
				if (id == ID_END || id == ID_FILES_INFO)
					break;		// The names aren't needed: the first file is the first stream
				if (id == ID_ARCHIVE_PROPERTIES)
				{
					for (;;)
					{
						const uint64_t property = reader.Number();
						if (property == 0 || reader.Bad())
							break;
						reader.Skip(reader.Number());
					}
				}
				else if (id == ID_ADDITIONAL_STREAMS_INFO)
				{
					StreamsInfo additional;
					if (!ReadStreamsInfo(reader, additional, error))
						return false;
				}
				else if (id == ID_MAIN_STREAMS_INFO)
				{
					if (!ReadStreamsInfo(reader, main, error))
						return false;
				}
				else
					return Fail(error, "Unexpected 7-zip header property " + std::to_string(id));
			}

			if (!DecodeFirstFolder(data, size, main, out, error))
				return false;
			const Folder &folder = main.folders.front();
			if (folder.unpackStreams > 1 && !folder.streamSizes.empty() && folder.streamSizes.front() < out.size())
				out.resize(static_cast<size_t>(folder.streamSizes.front()));
			return true;
		}
		return Fail(error, "7-zip headers nest too deep");
	}
}

#if defined(_MSC_VER) && !defined(__clang__) && !defined(NDEBUG)
#pragma optimize("", on)
#endif

// -------------------------------------------------------
