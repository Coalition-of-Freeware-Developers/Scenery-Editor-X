/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* animation.cpp
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#include <cmath>
#include <cstdint>
#include <vector>

#include "animation.h"
#include <algorithm>
#include <Math/includes/math_utils.h>
#include "SceneryEditorX/asset/managers/asset_manager.h"
#include "SceneryEditorX/asset/mesh/mesh.h"
#include "SceneryEditorX/core/memory/memory.h"
#include "SceneryEditorX/project/project.h"

/// -------------------------------------------------------


namespace SceneryEditorX
{
	/// Utilities for animation, math helpers, etc.
	namespace Utils
    {
	    namespace Animation
	    {
			Vec3 SafeNormalize(Vec3 v)
			{
				// Use native normalize that safely handles zero-length vectors
				return Normalize(v);
			}

			Vec3 TransformVector(const Transform& t, const Vec3& v)
			{
				return t.Rotation * (t.Scale * v);
			}

			Vec3 TransformPoint(const Transform& t, const Vec3& v)
			{
				return t.Rotation * (t.Scale * v) + t.Translation;
			}

			/// Retarget declaration is in header; implementation can be added when needed.
	    }
	}

	/// Internal helpers for keyframe sampling (engine-native, no ACL/RTM)
	namespace
	{
		inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t)
		{
            return a * (1.0f - t) + b * t;
		}

		inline Quat Slerp(const Quat& qa, const Quat& qb, float t)
		{
			float cosTheta = Quat::Dot(qa, qb);
			Quat b = qb;
			if (cosTheta < 0.0f) { b = -qb; cosTheta = -cosTheta; }
			if (cosTheta > 0.9995f)
			{
				Quat result(
					qa.w + t * (b.w - qa.w),
					qa.x + t * (b.x - qa.x),
					qa.y + t * (b.y - qa.y),
					qa.z + t * (b.z - qa.z));
				return result.normalize();
			}
			float theta = std::acos(std::clamp(cosTheta, -1.0f, 1.0f));
			float sinTheta = std::sin(theta);
			float w1 = std::sin((1.0f - t) * theta) / sinTheta;
			float w2 = std::sin(t * theta) / sinTheta;
			return Quat(
				w1 * qa.w + w2 * b.w,
				w1 * qa.x + w2 * b.x,
				w1 * qa.y + w2 * b.y,
				w1 * qa.z + w2 * b.z);
		}


		inline void FindBracketingKeys(const std::vector<AnimationInternal::KeyframeVec3>& keys, float time, uint32_t& outA, uint32_t& outB, float& outAlpha)
		{
			if (keys.empty()) { outA = outB = 0; outAlpha = 0.0f; return; }
			if (keys.size() == 1) { outA = outB = 0; outAlpha = 0.0f; return; }
			if (time <= keys.front().Time) { outA = 0; outB = 1; outAlpha = 0.0f; return; }
			if (time >= keys.back().Time) { outA = static_cast<uint32_t>(keys.size() - 2); outB = static_cast<uint32_t>(keys.size() - 1); outAlpha = 1.0f; return; }
			for (uint32_t i = 0; i + 1 < keys.size(); ++i)
			{
				if (time >= keys[i].Time && time <= keys[i + 1].Time)
				{
					outA = i;
					outB = i + 1;
					const float dt = keys[outB].Time - keys[outA].Time;
					outAlpha = dt > 0.0f ? (time - keys[outA].Time) / dt : 0.0f;
					return;
				}
			}
			outA = static_cast<uint32_t>(keys.size() - 2);
			outB = static_cast<uint32_t>(keys.size() - 1);
			outAlpha = 1.0f;
		}

		inline void FindBracketingKeys(const std::vector<AnimationInternal::KeyframeQuat>& keys, float time, uint32_t& outA, uint32_t& outB, float& outAlpha)
		{
			if (keys.empty()) { outA = outB = 0; outAlpha = 0.0f; return; }
			if (keys.size() == 1) { outA = outB = 0; outAlpha = 0.0f; return; }
			if (time <= keys.front().Time) { outA = 0; outB = 1; outAlpha = 0.0f; return; }
			if (time >= keys.back().Time) { outA = static_cast<uint32_t>(keys.size() - 2); outB = static_cast<uint32_t>(keys.size() - 1); outAlpha = 1.0f; return; }
			for (uint32_t i = 0; i + 1 < keys.size(); ++i)
			{
				if (time >= keys[i].Time && time <= keys[i + 1].Time)
				{
					outA = i;
					outB = i + 1;
					const float dt = keys[outB].Time - keys[outA].Time;
					outAlpha = dt > 0.0f ? (time - keys[outA].Time) / dt : 0.0f;
					return;
				}
			}
			outA = static_cast<uint32_t>(keys.size() - 2);
			outB = static_cast<uint32_t>(keys.size() - 1);
			outAlpha = 1.0f;
		}

		inline void SampleRootAtTime(const AnimationInternal::InternalAnimationData* data, float time, Vec3& outT, Quat& outR)
		{
			outT = Vec3(0.0f);
			outR = Quat(1.0f, 0.0f, 0.0f, 0.0f);
			if (!data || data->Tracks.empty()) return;

			const AnimationInternal::TrackTRS& root = data->Tracks[0];
			if (!root.Translations.empty())
			{
				uint32_t a, b; float alpha;
				FindBracketingKeys(root.Translations, time, a, b, alpha);
				const Vec3& va = root.Translations[a].Value;
				const Vec3& vb = root.Translations[b].Value;
				outT = Lerp(va, vb, alpha);
			}

			if (!root.Rotations.empty())
			{
				uint32_t a, b; float alpha;
				FindBracketingKeys(root.Rotations, time, a, b, alpha);
				const Quat& qa = root.Rotations[a].Value;
				const Quat& qb = root.Rotations[b].Value;
				outR = Slerp(qa, qb, alpha);
			}
		}
	}

	const Transform Transform::Identity = {};

	Animation::Animation(const Skeleton* skeleton, const float duration, const uint32_t numTracks, void* data)
		: m_Data(data)
		, m_Skeleton(skeleton)
		, m_RootTranslationEnd(0.0f, 0.0f, 0.0f)
		, m_Duration(duration)
	, m_RootRotationEnd(Quat(1.0f, 0.0f, 0.0f, 0.0f))
		, m_NumTracks(numTracks)
	{
		if (data)
		{
			const auto* internal = static_cast<const AnimationInternal::InternalAnimationData*>(data);
			const float t = internal ? std::clamp(m_Duration, 0.0f, internal->Duration) : m_Duration;
			SampleRootAtTime(internal, t, m_RootTranslationEnd, m_RootRotationEnd);
		}
	}

	Animation::Animation(Animation&& other) noexcept
		: m_Data(other.m_Data)
		, m_Skeleton(other.m_Skeleton)
		, m_RootTranslationEnd(other.m_RootTranslationEnd)
		, m_Duration(other.m_Duration)
		, m_RootRotationEnd(other.m_RootRotationEnd)
		, m_NumTracks(other.m_NumTracks)
	{
		other.m_Data = nullptr;
	}

	Animation& Animation::operator=(Animation&& other) noexcept
	{
		if (this != &other)
		{
			m_Data = other.m_Data;
			m_Skeleton = other.m_Skeleton;
			m_Duration = other.m_Duration;
			m_NumTracks = other.m_NumTracks;
			m_RootTranslationEnd = other.m_RootTranslationEnd;
			m_RootRotationEnd = other.m_RootRotationEnd;
			other.m_Data = nullptr;
		}
		return *this;
	}

	Animation::~Animation()
	{
		if (m_Data)
		{
			hdelete static_cast<AnimationInternal::InternalAnimationData*>(m_Data);
			m_Data = nullptr;
		}
	}

	uint32_t Animation::GetNumFrames() const
	{
		if (!m_Data) return 0;
		const auto* tracks = static_cast<const AnimationInternal::InternalAnimationData*>(m_Data);
		return tracks ? tracks->NumFrames : 0;
	}

	/// AnimationAsset methods (unchanged behavior)
    AnimationAsset::AnimationAsset(AssetHandle animationSource, AssetHandle mesh, const std::string_view animationName, bool extractRootMotion, uint32_t rootBoneIndex, const Vec3& rootTranslationMask, const Vec3& rootRotationMask, bool discardRootMotion)
		: m_RootTranslationMask(Bool3{ rootTranslationMask.x != 0.0f, rootTranslationMask.y != 0.0f, rootTranslationMask.z != 0.0f })
		, m_RootRotationMask(Bool3{ rootRotationMask.x != 0.0f, rootRotationMask.y != 0.0f, rootRotationMask.z != 0.0f })
		, m_AnimationSource(animationSource)
		, m_Mesh(mesh)
		, m_AnimationName(animationName)
		, m_RootBoneIndex(rootBoneIndex)
		, m_IsExtractRootMotion(extractRootMotion)
		, m_IsDiscardRootMotion(discardRootMotion)
	{
	SEDX_CORE_ASSERT(rootTranslationMask.x == 0.0f || rootTranslationMask.x == 1.0f);
	SEDX_CORE_ASSERT(rootTranslationMask.y == 0.0f || rootTranslationMask.y == 1.0f);
	SEDX_CORE_ASSERT(rootTranslationMask.z == 0.0f || rootTranslationMask.z == 1.0f);
	SEDX_CORE_ASSERT(rootRotationMask.x == 0.0f || rootRotationMask.x == 1.0f);
	SEDX_CORE_ASSERT(rootRotationMask.y == 0.0f || rootRotationMask.y == 1.0f);
	SEDX_CORE_ASSERT(rootRotationMask.z == 0.0f || rootRotationMask.z == 1.0f);
	}

	AssetHandle AnimationAsset::GetAnimationSource() const { return m_AnimationSource; }
	AssetHandle AnimationAsset::GetMeshHandle() const { return m_Mesh; }
	const std::string& AnimationAsset::GetAnimationName() const { return m_AnimationName; }
	bool AnimationAsset::IsExtractRootMotion() const { return m_IsExtractRootMotion; }
	uint32_t AnimationAsset::RootBoneIndex() const { return m_RootBoneIndex; }
	const Bool3& AnimationAsset::GetRootTranslationMask() const { return m_RootTranslationMask; }
	const Bool3& AnimationAsset::GetRootRotationMask() const { return m_RootRotationMask; }
	bool AnimationAsset::IsDiscardRootMotion() const { return m_IsDiscardRootMotion; }

	/*
	const SceneryEditorX::Animation* AnimationAsset::GetAnimation() const
	{
		auto animationSource = AssetManager::GetAsset<MeshSource>(m_AnimationSource);
		auto mesh = AssetManager::GetAsset<Mesh>(m_Mesh);
		if (animationSource && mesh && mesh->GetSkeleton())
		{
			return animationSource->GetAnimation(m_AnimationName, *mesh->GetSkeleton(), m_IsExtractRootMotion, m_RootBoneIndex, m_RootTranslationMask, m_RootRotationMask, m_IsDiscardRootMotion);
		}
		return nullptr;
	}
	*/

	/*
	void AnimationAsset::OnDependencyUpdated(AssetHandle)
	{
		Project::GetAssetManager()->ReloadDataAsync(Handle);
	}
	*/

}

/// -------------------------------------------------------

