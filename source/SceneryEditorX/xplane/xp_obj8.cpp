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
 * xp_obj8.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "xp_obj8.h"
#include "xp_text.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace
	{
		constexpr uint32_t MAX_WARNINGS = 64;

		/* Conditional (IF / ELSE / ENDIF) variables X-Plane 12 evaluates at load time; the editor emulates HDR with shadows. */
		bool EvaluateConditionVariable(const std::string_view name)
		{
			return name == "GLOBAL_LIGHTING" || name == "GLOBAL_SHADOWS" || name == "SCENERY_SHADOWS" || name == "VERSION10";
		}

		/**
		 * @class Obj8Parser
		 * @brief Single-pass OBJ8 command interpreter. Commands are processed in file order like X-Plane does.
		 */
		class Obj8Parser
		{
		public:
			explicit Obj8Parser(Obj8Data &data) : m_Data(data) {}

			bool Parse(std::string_view text, std::string *error)
			{
				LineReader reader(text);
				if (!ParseHeader(reader, error))
					return false;

				std::string_view line;
				std::vector<std::string_view> tokens;
				while (reader.Next(line))
				{
					Tokenize(line, tokens);
					if (tokens.empty() || tokens[0].front() == '#')
						continue;

					m_Line = reader.GetLineNumber();
					m_RawLine = line;

					if (HandleConditional(tokens))
						continue;

					if (!m_Active)
						continue;

					HandleCommand(tokens);
				}

				if (!m_BeginStack.empty())
					Warn("ANIM_begin without a matching ANIM_end at end of file");

				if (m_PendingOp)
					Warn("Keyframe table not closed with ANIM_*_end at end of file");

				if (!m_ConditionStack.empty())
					Warn("IF without a matching ENDIF at end of file");

				Validate();
				CollectDatarefs();
				return true;
			}

		private:
			struct BeginFrame
			{
				int node = -1;
				std::vector<uint32_t> visibility;
			};

			struct ConditionFrame
			{
				bool parentActive = true;	// Whether the enclosing block was active
				bool taken = false;			// A branch of this IF already ran
			};

			// -------------------------------------------------------

			bool ParseHeader(LineReader &reader, std::string *error)
			{
				std::string_view line;
				std::vector<std::string_view> tokens;

				// 1: A or I (X-Plane ignores the value), 2: version, 3: OBJ
				std::string_view header[3];
				for (std::string_view &entry : header)
				{
					do
					{
						if (!reader.Next(line))
						{
							SetError(error, "File is too short to contain an OBJ8 header");
							return false;
						}
						Tokenize(line, tokens);
					}
					while (tokens.empty());
					entry = tokens[0];
				}

				std::string_view origin = header[0];
				if (origin.size() >= 3 && static_cast<unsigned char>(origin[0]) == 0xEF) // UTF-8 BOM
					origin.remove_prefix(3);

				if (origin != "A" && origin != "I")
				{
					SetError(error, "Not an X-Plane object: the first line must be 'A' or 'I'");
					return false;
				}

				const int version = ParseInt(header[1], -1);
				if (version < 800)
				{
					SetError(error, version > 0 ? "OBJ version " + std::string(header[1]) + " is not supported (OBJ8 / 800 is required)"
												: "Missing OBJ version on the second line");
					return false;
				}
				m_Data.version = static_cast<uint32_t>(version);

				if (header[2] != "OBJ")
				{
					SetError(error, "Not an X-Plane object: the third line must be 'OBJ'");
					return false;
				}

				return true;
			}

			/* @brief IF / ELSE / ENDIF. Returns true when the line was a conditional. */
			bool HandleConditional(const std::vector<std::string_view> &tokens)
			{
				const std::string_view cmd = tokens[0];
				if (cmd == "IF")
				{
					ConditionFrame frame;
					frame.parentActive = m_Active;
					const bool value = EvaluateCondition(tokens, 1);
					frame.taken = value;
					m_ConditionStack.push_back(frame);
					m_Active = frame.parentActive && value;
					return true;
				}

				if (cmd == "ELSE")
				{
					if (m_ConditionStack.empty())
					{
						Warn("ELSE without IF");
						return true;
					}

					ConditionFrame &frame = m_ConditionStack.back();
					bool value = true;
					if (tokens.size() > 1 && tokens[1] == "IF")
						value = EvaluateCondition(tokens, 2);

					const bool run = !frame.taken && value;
					frame.taken |= run;
					m_Active = frame.parentActive && run;
					return true;
				}

				if (cmd == "ENDIF")
				{
					if (m_ConditionStack.empty())
					{
						Warn("ENDIF without IF");
						return true;
					}

					m_Active = m_ConditionStack.back().parentActive;
					m_ConditionStack.pop_back();
					return true;
				}

				return false;
			}

			static bool EvaluateCondition(const std::vector<std::string_view> &tokens, const size_t first)
			{
				size_t index = first;
				bool negate = false;
				if (index < tokens.size() && tokens[index] == "NOT")
				{
					negate = true;
					++index;
				}

				const bool value = index < tokens.size() && EvaluateConditionVariable(tokens[index]);
				return negate ? !value : value;
			}

			// -------------------------------------------------------

			void HandleCommand(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];

				// Data tables first: they make up most of the file.
				if (cmd == "VT")
				{
					Obj8Vertex v;
					if (t.size() >= 9)
					{
						for (int i = 0; i < 3; ++i)
						{
							v.position[i] = ParseFloat(t[1 + i]);
							v.normal[i] = ParseFloat(t[4 + i]);
						}
						v.uv[0] = ParseFloat(t[7]);
						v.uv[1] = ParseFloat(t[8]);
					}
					else
					{
						Warn("VT needs 8 values");
					}
					m_Data.vertices.push_back(v);
					return;
				}

				if (cmd == "IDX10" || cmd == "IDX")
				{
					for (size_t i = 1; i < t.size(); ++i)
						m_Data.indices.push_back(static_cast<uint32_t>(std::max(0, ParseInt(t[i], 0))));
					return;
				}

				if (cmd == "TRIS")
				{
					HandleTris(t);
					return;
				}

				if (cmd == "VLINE" || cmd == "VLIGHT")
				{
					Obj8Vertex v;
					for (int i = 0; i < 3 && i + 1 < static_cast<int>(t.size()); ++i)
						v.position[i] = ParseFloat(t[1 + i]);
					for (int i = 0; i < 3 && i + 4 < static_cast<int>(t.size()); ++i)
						v.normal[i] = ParseFloat(t[4 + i]);
					(cmd == "VLINE" ? m_Data.lineVertices : m_Data.lightVertices).push_back(v);
					return;
				}

				if (cmd.starts_with("ANIM_"))
				{
					HandleAnimation(t);
					return;
				}

				if (cmd.starts_with("ATTR_"))
				{
					HandleAttribute(t);
					return;
				}

				if (cmd.starts_with("LIGHT") || cmd == "LIGHTS")
				{
					HandleLight(t);
					return;
				}

				if (cmd.starts_with("TEXTURE") || cmd.starts_with("NORMAL") || cmd.starts_with("BLEND_"))
				{
					HandleTexture(t);
					return;
				}

				if (cmd == "SUPER_ROUGHNESS" || cmd.starts_with("WEATHER") || cmd == "SNOW_ALBEDO_LUMA" || cmd.starts_with("DECAL"))
				{
					HandleMaterial(t);
					return;
				}

				if (cmd.starts_with("GLOBAL_"))
				{
					HandleGlobal(t);
					return;
				}

				if (cmd == "POINT_COUNTS")
				{
					if (t.size() >= 5)
					{
						m_Data.vertices.reserve(static_cast<size_t>(std::max(0, ParseInt(t[1], 0))));
						m_Data.lineVertices.reserve(static_cast<size_t>(std::max(0, ParseInt(t[2], 0))));
						m_Data.lightVertices.reserve(static_cast<size_t>(std::max(0, ParseInt(t[3], 0))));
						m_Data.indices.reserve(static_cast<size_t>(std::max(0, ParseInt(t[4], 0))));
					}
					return;
				}

				if (cmd == "LINES")
				{
					++m_Data.linesCommandCount;
					return;
				}

				if (cmd == "TILTED")
				{
					m_Data.tilted = true;
					return;
				}

				if (cmd == "PARTICLE_SYSTEM")
				{
					m_Data.particleSystem = std::string(RestOfLine(m_RawLine, 1));
					return;
				}

				// Recognized commands the editor doesn't use (placement filters, weather, cockpit, particles, decals).
				static constexpr std::string_view IGNORED[] =
				{
					"SLOPE_LIMIT", "REQUIRE_WET", "REQUIRE_DRY", "slung_load_weight", "COCKPIT_REGION", "RAIN_scale",
					"RAIN_friction", "THERMAL_texture", "THERMAL_source", "THERMAL_source2", "WIPER_texture", "WIPER_param",
					"EMITTER", "MAGNET", "smoke_black", "smoke_white", "NO_BLEND", "TWO_SIDED", "SPECULAR", "BUMP_LEVEL", "NO_SHADOW",
					"DITHER_ALPHA", "NO_ALPHA", "LOAD_CENTER", "SHADOW_BLEND", "ATTACHMENT", "DEBUG", "EXPORT"
				};

				if (std::ranges::find(IGNORED, cmd) != std::end(IGNORED))
					return;

				++m_Data.unknownCommandCount;
				if (m_Data.unknownCommandCount <= 8)
					Warn("Unknown command '" + std::string(cmd) + "'");
			}

			// -------------------------------------------------------

			void HandleTris(const std::vector<std::string_view> &t)
			{
				++m_Data.trisCommandCount;
				if (t.size() < 3)
				{
					Warn("TRIS needs an offset and a count");
					return;
				}

				const int offset = ParseInt(t[1], -1);
				int count = ParseInt(t[2], -1);
				if (offset < 0 || count <= 0)
				{
					Warn("TRIS with an invalid range");
					return;
				}

				if (count % 3 != 0)
				{
					Warn("TRIS count is not a multiple of 3");
					count -= count % 3;
				}

				SealCurrentNode();

				const Obj8Batch::Range range{ static_cast<uint32_t>(offset), static_cast<uint32_t>(count) };
				if (!m_Data.batches.empty())
				{
					Obj8Batch &last = m_Data.batches.back();
					if (last.node == m_CurrentNode && last.lod == m_CurrentLod && last.state == m_State && last.visibility == m_CurrentVisibility)
					{
						Obj8Batch::Range &tail = last.ranges.back();
						if (tail.offset + tail.count == range.offset)
							tail.count += range.count;
						else
							last.ranges.push_back(range);
						return;
					}
				}

				Obj8Batch batch;
				batch.ranges.push_back(range);
				batch.node = m_CurrentNode;
				batch.lod = m_CurrentLod;
				batch.state = m_State;
				batch.visibility = m_CurrentVisibility;
				m_Data.batches.push_back(std::move(batch));
			}

			// -------------------------------------------------------

			void HandleAnimation(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];

				if (cmd == "ANIM_begin")
				{
					BeginFrame frame;
					frame.node = m_CurrentNode;
					frame.visibility = m_CurrentVisibility;
					m_BeginStack.push_back(std::move(frame));

					SealCurrentNode();
					m_CurrentNode = AddNode(m_CurrentNode, false);
					return;
				}

				if (cmd == "ANIM_end")
				{
					if (m_PendingOp)
					{
						Warn("ANIM_end inside an unfinished keyframe table");
						FinishPendingOp();
					}

					if (m_BeginStack.empty())
					{
						Warn("ANIM_end without ANIM_begin");
						return;
					}

					m_CurrentNode = m_BeginStack.back().node;
					m_CurrentVisibility = std::move(m_BeginStack.back().visibility);
					m_BeginStack.pop_back();
					return;
				}

				if (cmd == "ANIM_trans")
				{
					// ANIM_trans <x1> <y1> <z1> <x2> <y2> <z2> [<v1> <v2> [dataref]]
					// Exporters write static offsets without the calibration range (XPlane2Blender: six values).
					if (t.size() < 7)
					{
						Warn("ANIM_trans needs at least 6 values");
						return;
					}

					Obj8AnimOp op;
					op.type = Obj8AnimOp::Type::Translate;
					Obj8TransKey k1, k2;
					for (int i = 0; i < 3; ++i)
					{
						k1.offset[i] = ParseFloat(t[1 + i]);
						k2.offset[i] = ParseFloat(t[4 + i]);
					}
					k1.value = t.size() > 7 ? ParseFloat(t[7]) : 0.0f;
					k2.value = t.size() > 8 ? ParseFloat(t[8]) : 0.0f;
					op.transKeys = { k1, k2 };
					if (t.size() > 9)
						op.dataref = DatarefRef::Parse(t[9]);
					AddOp(std::move(op));
					return;
				}

				if (cmd == "ANIM_rotate")
				{
					// ANIM_rotate <x> <y> <z> <r1> <r2> [<v1> <v2> [dataref]]
					if (t.size() < 6)
					{
						Warn("ANIM_rotate needs at least 5 values");
						return;
					}

					Obj8AnimOp op;
					op.type = Obj8AnimOp::Type::Rotate;
					SetAxis(op, ParseFloat(t[1]), ParseFloat(t[2]), ParseFloat(t[3]));
					const float v1 = t.size() > 6 ? ParseFloat(t[6]) : 0.0f;
					const float v2 = t.size() > 7 ? ParseFloat(t[7]) : 0.0f;
					op.rotKeys = { Obj8RotKey{ v1, ParseFloat(t[4]) }, Obj8RotKey{ v2, ParseFloat(t[5]) } };
					if (t.size() > 8)
						op.dataref = DatarefRef::Parse(t[8]);
					AddOp(std::move(op));
					return;
				}

				if (cmd == "ANIM_trans_begin")
				{
					// ANIM_trans_begin <dataref>
					StartPendingOp(Obj8AnimOp::Type::Translate);
					if (t.size() > 1)
						m_PendingOp->dataref = DatarefRef::Parse(t[1]);
					return;
				}

				if (cmd == "ANIM_rotate_begin")
				{
					// ANIM_rotate_begin <x> <y> <z> <dataref>
					StartPendingOp(Obj8AnimOp::Type::Rotate);
					if (t.size() >= 4)
						SetAxis(*m_PendingOp, ParseFloat(t[1]), ParseFloat(t[2]), ParseFloat(t[3]));
					else
						Warn("ANIM_rotate_begin needs an axis");
					if (t.size() > 4)
						m_PendingOp->dataref = DatarefRef::Parse(t[4]);
					return;
				}

				if (cmd == "ANIM_trans_key")
				{
					// ANIM_trans_key <value> <x> <y> <z>
					if (!m_PendingOp || m_PendingOp->type != Obj8AnimOp::Type::Translate || t.size() < 5)
					{
						Warn("ANIM_trans_key outside ANIM_trans_begin / ANIM_trans_end");
						return;
					}

					Obj8TransKey key;
					key.value = ParseFloat(t[1]);
					for (int i = 0; i < 3; ++i)
						key.offset[i] = ParseFloat(t[2 + i]);
					m_PendingOp->transKeys.push_back(key);
					return;
				}

				if (cmd == "ANIM_rotate_key")
				{
					// ANIM_rotate_key <value> <angle>
					if (!m_PendingOp || m_PendingOp->type != Obj8AnimOp::Type::Rotate || t.size() < 3)
					{
						Warn("ANIM_rotate_key outside ANIM_rotate_begin / ANIM_rotate_end");
						return;
					}

					m_PendingOp->rotKeys.push_back(Obj8RotKey{ ParseFloat(t[1]), ParseFloat(t[2]) });
					return;
				}

				if (cmd == "ANIM_trans_end" || cmd == "ANIM_rotate_end")
				{
					if (!m_PendingOp)
					{
						Warn(std::string(cmd) + " without a matching *_begin");
						return;
					}
					FinishPendingOp();
					return;
				}

				if (cmd == "ANIM_keyframe_loop")
				{
					// Applies to the animation immediately before it: the table still being keyed, the last
					// transform, or the last hide / show rule (flashing lights loop their visibility).
					const float loop = t.size() > 1 ? ParseFloat(t[1]) : 0.0f;
					if (m_PendingOp)
						m_PendingOp->loop = loop;
					else if (m_LoopTarget == LoopTarget::Visibility && !m_Data.visibility.empty())
						m_Data.visibility.back().loop = loop;
					else if (m_LoopTarget == LoopTarget::Op && m_LastOpNode >= 0 && !m_Data.nodes[m_LastOpNode].ops.empty())
						m_Data.nodes[m_LastOpNode].ops.back().loop = loop;
					else
						Warn("ANIM_keyframe_loop without a preceding animation");
					return;
				}

				if (cmd == "ANIM_hide" || cmd == "ANIM_show")
				{
					// ANIM_hide <v1> <v2> <dataref>
					if (t.size() < 4)
					{
						Warn(std::string(cmd) + " needs v1 v2 dataref");
						return;
					}

					Obj8VisibilityCmd rule;
					rule.hide = cmd == "ANIM_hide";
					rule.v1 = ParseFloat(t[1]);
					rule.v2 = ParseFloat(t[2]);
					rule.dataref = DatarefRef::Parse(t[3]);
					m_CurrentVisibility.push_back(static_cast<uint32_t>(m_Data.visibility.size()));
					m_Data.visibility.push_back(std::move(rule));
					m_LoopTarget = LoopTarget::Visibility;
					return;
				}

				++m_Data.unknownCommandCount;
				Warn("Unknown animation command '" + std::string(cmd) + "'");
			}

			void StartPendingOp(const Obj8AnimOp::Type type)
			{
				if (m_PendingOp)
				{
					Warn("New keyframe table started before the previous one ended");
					FinishPendingOp();
				}
				m_PendingOp = std::make_unique<Obj8AnimOp>();
				m_PendingOp->type = type;
			}

			void FinishPendingOp()
			{
				Obj8AnimOp op = std::move(*m_PendingOp);
				m_PendingOp.reset();

				const size_t keyCount = op.type == Obj8AnimOp::Type::Translate ? op.transKeys.size() : op.rotKeys.size();
				if (keyCount == 0)
				{
					Warn("Keyframe table without keys ignored");
					return;
				}

				AddOp(std::move(op));
			}

			void AddOp(Obj8AnimOp &&op)
			{
				// Animation commands belong right after ANIM_begin. If geometry (or a child group) already used the current
				// group, the op only affects what follows, so it gets an implicit child group that ANIM_end unwinds.
				if (m_CurrentNode < 0)
				{
					Warn("Animation command outside ANIM_begin / ANIM_end");
					m_CurrentNode = AddNode(-1, true);
				}
				else if (m_NodeSealed[m_CurrentNode])
				{
					m_CurrentNode = AddNode(m_CurrentNode, true);
				}

				m_Data.nodes[m_CurrentNode].ops.push_back(std::move(op));
				m_LastOpNode = m_CurrentNode;
				m_LoopTarget = LoopTarget::Op;
			}

			int AddNode(const int parent, const bool implicit)
			{
				Obj8AnimNode node;
				node.parent = parent;
				node.implicit = implicit;
				m_Data.nodes.push_back(std::move(node));
				m_NodeSealed.push_back(false);
				return static_cast<int>(m_Data.nodes.size()) - 1;
			}

			void SealCurrentNode()
			{
				if (m_CurrentNode >= 0)
					m_NodeSealed[m_CurrentNode] = true;
			}

			void SetAxis(Obj8AnimOp &op, float x, float y, float z)
			{
				const float length = std::sqrt(x * x + y * y + z * z);
				if (length < 1e-8f)
				{
					Warn("Rotation with a zero-length axis; using +Y");
					x = 0.0f;
					y = 1.0f;
					z = 0.0f;
				}
				else
				{
					x /= length;
					y /= length;
					z /= length;
				}
				op.axis[0] = x;
				op.axis[1] = y;
				op.axis[2] = z;
			}

			// -------------------------------------------------------

			void HandleAttribute(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];
				const auto arg = [&t](const size_t i, const float fallback) { return i < t.size() ? ParseFloat(t[i], fallback) : fallback; };

				if (cmd == "ATTR_LOD")
				{
					Obj8Lod lod;
					lod.nearDistance = arg(1, 0.0f);
					lod.farDistance = arg(2, 0.0f);
					if (lod.farDistance <= lod.nearDistance)
						Warn("ATTR_LOD with far <= near");

					if (!m_BeginStack.empty())
					{
						Warn("ATTR_LOD inside an animation group (groups must balance within each LOD)");
						m_BeginStack.clear();
						m_CurrentVisibility.clear();
					}

					m_CurrentNode = -1;
					m_Data.lods.push_back(lod);
					m_CurrentLod = static_cast<int>(m_Data.lods.size()) - 1;
					return;
				}

				if (cmd == "ATTR_no_blend")				{ m_State.blend = false; m_State.shadowBlend = false; m_State.alphaCutoff = arg(1, 0.5f); return; }
				if (cmd == "ATTR_blend")				{ m_State.blend = true; m_State.shadowBlend = false; return; }
				if (cmd == "ATTR_shadow_blend")			{ m_State.blend = true; m_State.shadowBlend = true; m_State.alphaCutoff = arg(1, 0.5f); return; }
				if (cmd == "ATTR_cull")					{ m_State.cull = true; return; }
				if (cmd == "ATTR_no_cull")				{ m_State.cull = false; return; }
				if (cmd == "ATTR_depth")				{ m_State.depth = true; return; }
				if (cmd == "ATTR_no_depth")				{ m_State.depth = false; return; }
				if (cmd == "ATTR_draped")				{ m_State.draped = true; return; }
				if (cmd == "ATTR_no_draped")			{ m_State.draped = false; return; }
				if (cmd == "ATTR_shadow")				{ m_State.castShadow = true; return; }
				if (cmd == "ATTR_no_shadow")			{ m_State.castShadow = false; return; }
				if (cmd == "ATTR_shade_flat")			{ m_State.flatShading = true; return; }
				if (cmd == "ATTR_shade_smooth")			{ m_State.flatShading = false; return; }
				if (cmd == "ATTR_solid_camera")			{ m_State.solidCamera = true; return; }
				if (cmd == "ATTR_no_solid_camera")		{ m_State.solidCamera = false; return; }
				if (cmd == "ATTR_draw_enable")			{ m_State.drawEnabled = true; return; }
				if (cmd == "ATTR_draw_disable")			{ m_State.drawEnabled = false; return; }
				if (cmd == "ATTR_poly_os")				{ m_State.polygonOffset = static_cast<int>(arg(1, 0.0f)); return; }
				if (cmd == "ATTR_shiny_rat")			{ m_State.shinyRatio = arg(1, 0.0f); return; }

				if (cmd == "ATTR_hard" || cmd == "ATTR_hard_deck")
				{
					m_State.hard = true;
					m_State.hardDeck = cmd == "ATTR_hard_deck";
					m_State.hardSurface = t.size() > 1 ? std::string(t[1]) : std::string();
					return;
				}

				if (cmd == "ATTR_no_hard")
				{
					m_State.hard = false;
					m_State.hardDeck = false;
					m_State.hardSurface.clear();
					return;
				}

				if (cmd == "ATTR_light_level")
				{
					// ATTR_light_level <v1> <v2> <dataref> [brightness]
					if (t.size() < 4)
					{
						Warn("ATTR_light_level needs v1 v2 dataref");
						return;
					}
					m_State.hasLightLevel = true;
					m_State.lightLevelV1 = arg(1, 0.0f);
					m_State.lightLevelV2 = arg(2, 1.0f);
					m_State.lightLevelDataref = DatarefRef::Parse(t[3]);
					m_State.lightLevelBrightness = arg(4, 0.0f);
					return;
				}

				if (cmd == "ATTR_light_level_reset")
				{
					m_State.hasLightLevel = false;
					m_State.lightLevelDataref = {};
					m_State.lightLevelBrightness = 0.0f;
					return;
				}

				if (cmd == "ATTR_reset")
				{
					// Restores the default material (shininess, emission); blending and culling are separate state.
					m_State.shinyRatio = 0.0f;
					m_State.hasLightLevel = false;
					m_State.lightLevelDataref = {};
					return;
				}

				if (cmd == "ATTR_cockpit" || cmd == "ATTR_cockpit_lit_only" || cmd == "ATTR_cockpit_region" || cmd == "ATTR_cockpit_device")
				{
					m_State.cockpit = true;
					return;
				}

				if (cmd == "ATTR_no_cockpit")
				{
					m_State.cockpit = false;
					return;
				}

				if (cmd == "ATTR_layer_group" || cmd == "ATTR_layer_group_draped")
				{
					Obj8LayerGroup group;
					group.name = t.size() > 1 ? std::string(t[1]) : std::string("objects");
					group.offset = static_cast<int>(arg(2, 0.0f));
					(cmd == "ATTR_layer_group" ? m_Data.layerGroup : m_Data.layerGroupDraped) = std::move(group);
					return;
				}

				if (cmd == "ATTR_LOD_draped")
				{
					m_Data.lodDraped = arg(1, 0.0f);
					return;
				}

				if (cmd.starts_with("ATTR_manip_"))
				{
					++m_Data.manipulatorCount;
					return;
				}

				static constexpr std::string_view IGNORED[] =
				{
					"ATTR_axis_detented", "ATTR_axis_detent_range", "ATTR_emission_rgb", "ATTR_ambient_rgb", "ATTR_specular_rgb",
					"ATTR_diffuse_rgb", "ATTR_hud_glass", "ATTR_hud_reset", "ATTR_landing_gear", "ATTR_offset", "ATTR_albedo_opacity",
					"ATTR_albedo_opacity_reset"
				};

				if (std::ranges::find(IGNORED, cmd) != std::end(IGNORED))
					return;

				++m_Data.unknownCommandCount;
				Warn("Unknown attribute '" + std::string(cmd) + "'");
			}

			// -------------------------------------------------------

			void HandleLight(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];

				const auto make_light = [this](const Obj8Light::Kind kind)
				{
					Obj8Light light;
					light.kind = kind;
					light.node = m_CurrentNode;
					light.lod = m_CurrentLod;
					light.visibility = m_CurrentVisibility;
					return light;
				};

				if (cmd == "LIGHTS")
				{
					// LIGHTS <offset> <count>: legacy VLIGHT entries, not indexed
					const int offset = t.size() > 1 ? ParseInt(t[1], -1) : -1;
					const int count = t.size() > 2 ? ParseInt(t[2], 0) : 0;
					if (offset < 0 || count <= 0)
					{
						Warn("LIGHTS with an invalid range");
						return;
					}

					SealCurrentNode();
					for (int i = 0; i < count; ++i)
					{
						Obj8Light light = make_light(Obj8Light::Kind::Legacy);
						light.params = { static_cast<float>(offset + i) }; // Resolved against lightVertices in Validate()
						m_Data.lights.push_back(std::move(light));
					}
					return;
				}

				if (cmd == "LIGHT_NAMED" || cmd == "LIGHT_PARAM")
				{
					// LIGHT_NAMED <name> <x> <y> <z> / LIGHT_PARAM <name> <x> <y> <z> [params...]
					if (t.size() < 5)
					{
						Warn(std::string(cmd) + " needs a name and a position");
						return;
					}

					SealCurrentNode();
					Obj8Light light = make_light(cmd == "LIGHT_NAMED" ? Obj8Light::Kind::Named : Obj8Light::Kind::Param);
					light.name = std::string(t[1]);
					for (int i = 0; i < 3; ++i)
						light.position[i] = ParseFloat(t[2 + i]);
					for (size_t i = 5; i < t.size(); ++i)
					{
						// X-Plane 12 writes photometric intensities with a unit ("150cd"); the value keeps its slot.
						std::string_view word = t[i];
						if (word.size() > 2 && word.ends_with("cd"))
							word.remove_suffix(2);

						float value = 0.0f;
						if (TryParseFloat(word, value))
							light.params.push_back(value);
						else
							light.extraArgs.emplace_back(t[i]);
					}
					m_Data.lights.push_back(std::move(light));
					return;
				}

				if (cmd == "LIGHT_CUSTOM" || cmd == "LIGHT_SPILL_CUSTOM")
				{
					// LIGHT_CUSTOM <x> <y> <z> <r> <g> <b> <a> <s> <s1> <t1> <s2> <t2> <dataref>
					// LIGHT_SPILL_CUSTOM <x> <y> <z> <r> <g> <b> <a> <s> <dx> <dy> <dz> <semi> <dref>
					if (t.size() < 13)
					{
						Warn(std::string(cmd) + " needs 12 values and a dataref");
						return;
					}

					SealCurrentNode();
					Obj8Light light = make_light(cmd == "LIGHT_CUSTOM" ? Obj8Light::Kind::Custom : Obj8Light::Kind::SpillCustom);
					for (int i = 0; i < 3; ++i)
						light.position[i] = ParseFloat(t[1 + i]);
					for (size_t i = 4; i < 13; ++i)
						light.params.push_back(ParseFloat(t[i]));
					if (t.size() > 13)
						light.dataref = DatarefRef::Parse(t[13]);
					m_Data.lights.push_back(std::move(light));
					return;
				}

				++m_Data.unknownCommandCount;
				Warn("Unknown light command '" + std::string(cmd) + "'");
			}

			// -------------------------------------------------------

			void HandleTexture(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];

				// TEXTURE_NORMAL may carry a ratio before the path (the standard-shader form); paths may contain spaces.
				const auto read_path = [this, &t](float *ratio) -> std::string
				{
					size_t first = 1;
					float value = 0.0f;
					if (ratio && t.size() > 2 && TryParseFloat(t[1], value))
					{
						*ratio = value;
						first = 2;
					}
					return std::string(RestOfLine(m_RawLine, first));
				};

				if (cmd == "TEXTURE" || cmd == "TEXTURE_NOWRAP")							{ m_Data.texture = read_path(nullptr); return; }
				if (cmd == "TEXTURE_LIT" || cmd == "TEXTURE_LIT_NOWRAP")					{ m_Data.textureLit = read_path(nullptr); return; }
				if (cmd == "TEXTURE_NORMAL" || cmd == "TEXTURE_NORMAL_NOWRAP")				{ m_Data.textureNormal = read_path(&m_Data.normalRatio); m_LastNormalDraped = false; return; }
				if (cmd == "TEXTURE_DRAPED")												{ m_Data.textureDraped = read_path(nullptr); return; }
				if (cmd == "TEXTURE_DRAPED_NORMAL")
				{
					float ratio = 1.0f;
					m_Data.textureDrapedNormal = read_path(&ratio);
					m_LastNormalDraped = true;
					return;
				}

				// NORMAL_METALNESS, also as shipped misspelled ("NORMAL_metalness", "NORMAL METALNESS")
				if (cmd == "NORMAL_METALNESS" || cmd == "NORMAL_metalness" || (cmd == "NORMAL" && t.size() > 1 && t[1] == "METALNESS"))
				{
					(m_LastNormalDraped ? m_Data.drapedNormalMetalness : m_Data.normalMetalness) = true;
					return;
				}

				if (cmd == "NORMAL_TRANSLUCENCY")
				{
					m_Data.normalTranslucency = true;
					return;
				}

				if (cmd == "NORMAL_DECAL_PARAMS")
				{
					HandleMaterial(t);
					return;
				}

				if (cmd == "BLEND_GLASS")
				{
					m_Data.blendGlass = true;
					return;
				}

				if (cmd == "BLEND_HASH")
				{
					m_Data.blendHash = true;
					m_Data.blendHashRatio = t.size() > 1 ? ParseFloat(t[1], 0.0f) : 0.0f;
					return;
				}

				if (cmd == "TEXTURE_CONTROL")												{ m_Data.textureControl = read_path(nullptr); return; }
				if (cmd == "TEXTURE_MODULATOR")												{ m_Data.textureModulator = read_path(nullptr); return; }
				if (cmd == "TEXTURE_TERRAIN")
				{
					// TEXTURE_TERRAIN <scale x> <scale z> <texture>
					if (t.size() < 4)
					{
						Warn("TEXTURE_TERRAIN needs two scales and a texture");
						return;
					}
					m_Data.textureTerrainScale[0] = ParseFloat(t[1], 0.0f);
					m_Data.textureTerrainScale[1] = ParseFloat(t[2], 0.0f);
					m_Data.textureTerrain = std::string(RestOfLine(m_RawLine, 3));
					return;
				}

				if (cmd == "TEXTURE_MAP")
				{
					// TEXTURE_MAP <usage> <path>, usage: normal | material_gloss | gloss
					if (t.size() < 3)
					{
						Warn("TEXTURE_MAP needs a usage and a path");
						return;
					}

					const std::string path(RestOfLine(m_RawLine, 2));
					if (t[1] == "normal")
					{
						m_Data.textureNormal = path;
						m_LastNormalDraped = false;
					}
					else if (t[1] == "material_gloss")
					{
						m_Data.textureMaterialGloss = path;
					}
					else if (t[1] == "gloss")
					{
						m_Data.textureGloss = path;
					}
					else
					{
						Warn("Unknown TEXTURE_MAP usage '" + std::string(t[1]) + "'");
					}
					return;
				}

				++m_Data.unknownCommandCount;
				Warn("Unknown texture command '" + std::string(cmd) + "'");
			}

			/* @brief X-Plane 12 material directives: roughness floor, weather receptivity, decals. */
			void HandleMaterial(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];

				if (cmd == "SUPER_ROUGHNESS")
				{
					m_Data.superRoughness = t.size() > 1 ? ParseFloat(t[1], 0.0f) : 0.0f;
					return;
				}

				if (cmd == "WEATHER_TRANSPARENT")
				{
					m_Data.weatherTransparent = true;
					return;
				}

				if (cmd == "WEATHER")
				{
					m_Data.weatherMask = std::string(RestOfLine(m_RawLine, 1));
					return;
				}

				if (cmd == "SNOW_ALBEDO_LUMA")
				{
					if (t.size() < 5)
					{
						Warn("SNOW_ALBEDO_LUMA needs four values");
						return;
					}
					m_Data.hasSnowAlbedoLuma = true;
					for (size_t i = 0; i < 4; ++i)
						m_Data.snowAlbedoLuma[i] = ParseFloat(t[i + 1], 0.0f);
					return;
				}

				if (cmd == "DECAL_LIB")
				{
					m_Data.decalLibrary = std::string(RestOfLine(m_RawLine, 1));
					return;
				}

				if (cmd == "DECAL" || cmd == "DECAL_RGBA" || cmd == "DECAL_KEYED" || cmd == "DECAL_PARAMS" || cmd == "DECAL_PARAMS_PROJ" ||
					cmd == "NORMAL_DECAL_PARAMS")
				{
					// <numbers...> <texture> [numbers...]: their meaning differs per directive (plan TASK-035 reads them)
					Obj8Decal decal;
					decal.directive = std::string(cmd);
					size_t first = 1;
					float value = 0.0f;
					while (first < t.size() && TryParseFloat(t[first], value))
					{
						decal.values.push_back(value);
						++first;
					}
					size_t last = t.size();
					while (last > first + 1 && TryParseFloat(t[last - 1], value))
						--last;
					for (size_t i = last; i < t.size(); ++i)
						decal.trailing.push_back(ParseFloat(t[i], 0.0f));
					if (first < last)
					{
						// The texture runs from its first token to the end of its last (paths may contain spaces)
						const std::string_view rest = RestOfLine(m_RawLine, first);
						const size_t end = static_cast<size_t>(t[last - 1].data() + t[last - 1].size() - rest.data());
						decal.texture = std::string(rest.substr(0, std::min(end, rest.size())));
					}
					m_Data.decals.push_back(std::move(decal));
					return;
				}

				++m_Data.unknownCommandCount;
				Warn("Unknown material command '" + std::string(cmd) + "'");
			}

			void HandleGlobal(const std::vector<std::string_view> &t)
			{
				const std::string_view cmd = t[0];
				const float value = t.size() > 1 ? ParseFloat(t[1], 0.0f) : 0.0f;

				if (cmd == "GLOBAL_no_blend")							{ m_Data.hasGlobalNoBlend = true; m_Data.globalNoBlend = t.size() > 1 ? value : 0.5f; return; }
				if (cmd == "GLOBAL_shadow_blend")						{ m_Data.hasGlobalShadowBlend = true; m_Data.globalShadowBlend = t.size() > 1 ? value : 0.5f; return; }
				if (cmd == "GLOBAL_specular")							{ m_Data.hasGlobalSpecular = true; m_Data.globalSpecular = value; return; }
				if (cmd == "GLOBAL_luminance" || cmd == "GLOBAL_luminence") { m_Data.globalLuminance = value; return; }
				if (cmd == "GLOBAL_no_shadow")							{ m_Data.globalNoShadow = true; return; }
				if (cmd == "GLOBAL_cockpit_lit")						{ m_Data.globalCockpitLit = true; return; }
				if (cmd == "GLOBAL_LOD")								{ return; } // Third-party exporter extension

				if (cmd == "GLOBAL_tint")
				{
					m_Data.globalTintAlbedo = value;
					m_Data.globalTintEmissive = t.size() > 2 ? ParseFloat(t[2], 0.0f) : 0.0f;
					return;
				}

				++m_Data.unknownCommandCount;
				Warn("Unknown global '" + std::string(cmd) + "'");
			}

			// -------------------------------------------------------

			/* @brief Drops out-of-range TRIS ranges and clamps out-of-range indices so the GPU builder never reads past the tables. */
			void Validate()
			{
				const uint32_t indexCount = static_cast<uint32_t>(m_Data.indices.size());
				for (Obj8Batch &batch : m_Data.batches)
				{
					std::erase_if(batch.ranges, [&](const Obj8Batch::Range &range)
					{
						if (range.offset + range.count <= indexCount)
							return false;
						Warn("TRIS range past the end of the index table dropped");
						return true;
					});
				}
				std::erase_if(m_Data.batches, [](const Obj8Batch &batch) { return batch.ranges.empty(); });

				const uint32_t vertexCount = static_cast<uint32_t>(m_Data.vertices.size());
				uint32_t clamped = 0;
				for (uint32_t &index : m_Data.indices)
				{
					if (index >= vertexCount)
					{
						index = 0;
						++clamped;
					}
				}

				if (clamped > 0)
					Warn(std::to_string(clamped) + " indices referenced missing vertices");

				// Legacy LIGHTS: pull position and color from the VLIGHT table.
				for (Obj8Light &light : m_Data.lights)
				{
					if (light.kind != Obj8Light::Kind::Legacy || light.params.empty())
						continue;

					const size_t index = static_cast<size_t>(light.params[0]);
					light.params.clear();
					if (index < m_Data.lightVertices.size())
					{
						const Obj8Vertex &v = m_Data.lightVertices[index];
						std::copy_n(v.position, 3, light.position);
						light.params = { v.normal[0], v.normal[1], v.normal[2] };
					}
				}

				if (m_Data.texture.empty())
					Warn("No TEXTURE: the object draws with a gray proxy texture");
			}

			/* @brief Builds the dataref table (with keyframe ranges) the editor shows as animation controls. */
			void CollectDatarefs()
			{
				std::map<std::string, Obj8DatarefUsage> usages;

				const auto note = [&usages](const DatarefRef &ref, const float lo, const float hi, const float loop)
				{
					if (!ref.IsSet())
						return;

					auto [it, inserted] = usages.try_emplace(ref.ToString());
					Obj8DatarefUsage &usage = it->second;
					if (inserted)
					{
						usage.dataref = ref;
						usage.minValue = lo;
						usage.maxValue = hi;
					}
					else
					{
						usage.minValue = std::min(usage.minValue, lo);
						usage.maxValue = std::max(usage.maxValue, hi);
					}
					usage.loop = std::max(usage.loop, loop);
					++usage.useCount;
				};

				for (const Obj8AnimNode &node : m_Data.nodes)
				{
					for (const Obj8AnimOp &op : node.ops)
					{
						float lo = 0.0f, hi = 0.0f;
						bool first = true;
						const auto extend = [&](const float v)
						{
							lo = first ? v : std::min(lo, v);
							hi = first ? v : std::max(hi, v);
							first = false;
						};

						for (const Obj8TransKey &key : op.transKeys)
							extend(key.value);
						for (const Obj8RotKey &key : op.rotKeys)
							extend(key.value);

						if (op.loop > 0.0f)
						{
							lo = std::min(lo, 0.0f);
							hi = std::max(hi, op.loop);
						}
						note(op.dataref, lo, hi, op.loop);
					}
				}

				for (const Obj8VisibilityCmd &rule : m_Data.visibility)
				{
					const float lo = rule.loop > 0.0f ? 0.0f : std::min(rule.v1, rule.v2);
					const float hi = rule.loop > 0.0f ? std::max(rule.loop, std::max(rule.v1, rule.v2)) : std::max(rule.v1, rule.v2);
					note(rule.dataref, lo, hi, rule.loop);
				}

				for (const Obj8Batch &batch : m_Data.batches)
				{
					if (batch.state.hasLightLevel)
						note(batch.state.lightLevelDataref, std::min(batch.state.lightLevelV1, batch.state.lightLevelV2), std::max(batch.state.lightLevelV1, batch.state.lightLevelV2), 0.0f);
				}

				m_Data.datarefs.clear();
				m_Data.datarefs.reserve(usages.size());
				for (auto &[name, usage] : usages)
					m_Data.datarefs.push_back(std::move(usage));
			}

			// -------------------------------------------------------

			void Warn(const std::string &message)
			{
				if (m_Data.warnings.size() < MAX_WARNINGS)
					m_Data.warnings.push_back("line " + std::to_string(m_Line) + ": " + message);
			}

			static void SetError(std::string *error, const std::string &message)
			{
				if (error)
					*error = message;
			}

			Obj8Data &m_Data;
			uint32_t m_Line = 0;
			std::string_view m_RawLine;

			int m_CurrentNode = -1;
			int m_CurrentLod = -1;
			std::vector<uint32_t> m_CurrentVisibility;
			std::vector<BeginFrame> m_BeginStack;
			std::vector<bool> m_NodeSealed;
			std::unique_ptr<Obj8AnimOp> m_PendingOp;
			int m_LastOpNode = -1;

			enum class LoopTarget : uint8_t
			{
				None,
				Op,
				Visibility
			};
			LoopTarget m_LoopTarget = LoopTarget::None;

			Obj8DrawState m_State;
			bool m_LastNormalDraped = false;

			std::vector<ConditionFrame> m_ConditionStack;
			bool m_Active = true;
		};

		// -------------------------------------------------------

		void RotateVector(const float q[4], const float v[3], float out[3])
		{
			// v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + q.w * v)
			const float cx = q[1] * v[2] - q[2] * v[1] + q[3] * v[0];
			const float cy = q[2] * v[0] - q[0] * v[2] + q[3] * v[1];
			const float cz = q[0] * v[1] - q[1] * v[0] + q[3] * v[2];
			out[0] = v[0] + 2.0f * (q[1] * cz - q[2] * cy);
			out[1] = v[1] + 2.0f * (q[2] * cx - q[0] * cz);
			out[2] = v[2] + 2.0f * (q[0] * cy - q[1] * cx);
		}

		/* @brief Locates the key segment for a value: returns i and the (possibly extrapolated) fraction between keys i and i + 1. */
		template<typename Key>
		void FindSegment(const std::vector<Key> &keys, const float value, size_t &index, float &fraction)
		{
			const size_t count = keys.size();
			index = 0;
			fraction = 0.0f;
			if (count < 2)
				return;

			if (value <= keys.front().value)
				index = 0;
			else if (value >= keys.back().value)
				index = count - 2;
			else
			{
				while (index + 2 < count && value > keys[index + 1].value)
					++index;
			}

			const float span = keys[index + 1].value - keys[index].value;
			fraction = std::abs(span) > 1e-12f ? (value - keys[index].value) / span : 0.0f;
		}
	}

	// -------------------------------------------------------

	DatarefRef DatarefRef::Parse(std::string_view text)
	{
		DatarefRef ref;
		text = Trim(text);
		if (text.empty() || text == "none" || text == "no_ref" || text == "NULL" || text == "null")
			return ref;

		if (const size_t open = text.rfind('['); open != std::string_view::npos && text.back() == ']')
		{
			const int index = ParseInt(text.substr(open + 1, text.size() - open - 2), -1);
			if (index >= 0)
			{
				ref.index = index;
				text = text.substr(0, open);
			}
		}

		ref.name = std::string(text);
		return ref;
	}

	std::string DatarefRef::ToString() const
	{
		return index >= 0 ? name + "[" + std::to_string(index) + "]" : name;
	}

	uint32_t Obj8Batch::GetIndexCount() const
	{
		uint32_t total = 0;
		for (const Range &range : ranges)
			total += range.count;
		return total;
	}

	bool Obj8Data::HasAdditiveLods() const
	{
		return std::ranges::all_of(lods, [](const Obj8Lod &lod) { return lod.nearDistance <= 0.0f; });
	}

	uint32_t Obj8Data::GetTriangleCount() const
	{
		uint32_t total = 0;
		for (const Obj8Batch &batch : batches)
			total += batch.GetIndexCount() / 3;
		return total;
	}

	bool ParseObj8(const std::string_view text, Obj8Data &out, std::string *error)
	{
		std::filesystem::path path = std::move(out.filePath);
		out = Obj8Data{};
		out.filePath = std::move(path);

		Obj8Parser parser(out);
		return parser.Parse(text, error);
	}

	bool LoadObj8(const std::filesystem::path &path, Obj8Data &out, std::string *error)
	{
		std::string text;
		if (!ReadTextFile(path, text))
		{
			if (error)
				*error = "Could not read '" + path.string() + "'";
			return false;
		}

		out.filePath = path;
		return ParseObj8(text, out, error);
	}

	// -------------------------------------------------------

	RigidTransform RigidTransform::Compose(const RigidTransform &inner) const
	{
		RigidTransform result;
		const float *a = rotation;
		const float *b = inner.rotation;
		result.rotation[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
		result.rotation[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
		result.rotation[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
		result.rotation[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];

		float rotated[3];
		RotateVector(rotation, inner.translation, rotated);
		for (int i = 0; i < 3; ++i)
			result.translation[i] = rotated[i] + translation[i];
		return result;
	}

	void RigidTransform::TransformPoint(const float in[3], float out[3]) const
	{
		float rotated[3];
		RotateVector(rotation, in, rotated);
		for (int i = 0; i < 3; ++i)
			out[i] = rotated[i] + translation[i];
	}

	float ApplyKeyframeLoop(const float value, const float loop)
	{
		if (loop <= 0.0f)
			return value;

		float reduced = std::fmod(value, loop);
		if (reduced < 0.0f)
			reduced += loop;
		return reduced;
	}

	RigidTransform EvaluateAnimOp(const Obj8AnimOp &op, float datarefValue)
	{
		RigidTransform result;
		datarefValue = ApplyKeyframeLoop(datarefValue, op.loop);

		if (op.type == Obj8AnimOp::Type::Translate)
		{
			if (op.transKeys.empty())
				return result;

			// A static op (no dataref) sits at its first key.
			if (!op.dataref.IsSet() || op.transKeys.size() == 1)
			{
				std::copy_n(op.transKeys.front().offset, 3, result.translation);
				return result;
			}

			size_t index = 0;
			float fraction = 0.0f;
			FindSegment(op.transKeys, datarefValue, index, fraction);
			const Obj8TransKey &k0 = op.transKeys[index];
			const Obj8TransKey &k1 = op.transKeys[index + 1];
			for (int i = 0; i < 3; ++i)
				result.translation[i] = k0.offset[i] + (k1.offset[i] - k0.offset[i]) * fraction;
			return result;
		}

		if (op.rotKeys.empty())
			return result;

		float angle = op.rotKeys.front().angleDeg;
		if (op.dataref.IsSet() && op.rotKeys.size() > 1)
		{
			size_t index = 0;
			float fraction = 0.0f;
			FindSegment(op.rotKeys, datarefValue, index, fraction);
			const Obj8RotKey &k0 = op.rotKeys[index];
			const Obj8RotKey &k1 = op.rotKeys[index + 1];
			angle = k0.angleDeg + (k1.angleDeg - k0.angleDeg) * fraction;
		}

		// Right-hand rule: positive angles turn counter-clockwise when looking from the axis tip toward the origin.
		constexpr float degToRad = 3.14159265358979323846f / 180.0f;
		const float half = angle * degToRad * 0.5f;
		const float s = std::sin(half);
		result.rotation[0] = op.axis[0] * s;
		result.rotation[1] = op.axis[1] * s;
		result.rotation[2] = op.axis[2] * s;
		result.rotation[3] = std::cos(half);
		return result;
	}

	RigidTransform EvaluateAnimNode(const Obj8AnimNode &node, const DatarefReader &reader)
	{
		// Ops apply like a matrix stack: the first op is the outermost transform, so the last op touches vertices first.
		RigidTransform result;
		for (const Obj8AnimOp &op : node.ops)
		{
			const float value = op.dataref.IsSet() && reader ? reader(op.dataref) : 0.0f;
			result = result.Compose(EvaluateAnimOp(op, value));
		}
		return result;
	}

	bool EvaluateVisibility(const Obj8Data &data, const std::vector<uint32_t> &rules, const DatarefReader &reader)
	{
		bool visible = true;
		for (const uint32_t ruleIndex : rules)
		{
			if (ruleIndex >= data.visibility.size())
				continue;

			const Obj8VisibilityCmd &rule = data.visibility[ruleIndex];
			const float value = ApplyKeyframeLoop(rule.dataref.IsSet() && reader ? reader(rule.dataref) : 0.0f, rule.loop);
			const float lo = std::min(rule.v1, rule.v2);
			const float hi = std::max(rule.v1, rule.v2);
			if (value >= lo && value <= hi)
				visible = !rule.hide;
		}
		return visible;
	}

}

// -------------------------------------------------------
