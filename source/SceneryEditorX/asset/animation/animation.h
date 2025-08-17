/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* animation.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <vector>
#include <Math/includes/math_utils.h>
#include <Math/includes/matrix.h>
#include <Math/includes/quat.h>
#include <Math/includes/transforms.h>
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/asset/asset_types.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class MeshSource;
	class Skeleton;

	/// Animation is a collection of keyed values for translation, rotation, and scale of a number of "tracks"
	/// Typically one "track" = one bone of a skeleton.
	/// (but later we may also want to animate other things, so a "track" isn't necessarily a bone)
	class Animation
	{
	public:
		Animation() = default;
		Animation(const Skeleton* skeleton, float duration = 0.0f, uint32_t numTracks = 0, void* data = nullptr);

		Animation(const Animation&) = delete; /// copying animation is not allowed (because we don't want to copy the data)
		Animation& operator=(const Animation&) = delete;

		Animation(Animation&& other) noexcept;
		Animation& operator=(Animation&& other) noexcept;

		~Animation();

		/// skeleton that the data animates (this is used for retargeting if the animation is applied to a different skeleton)
		/// (future: could be nullptr if the animation is not associated with a skeleton - e.g. animating a camera)
		const Skeleton* GetSkeleton() const { return m_Skeleton; }
		void SetSkeleton(const Skeleton* skeleton) { m_Skeleton = skeleton; }

		float GetDuration() const { return m_Duration; }
		uint32_t GetNumTracks() const { return m_NumTracks; }

		uint32_t GetNumFrames() const;

		/// Opaque animation data. Type-erased so we do not leak implementation headers into the engine.
		/// NOTE: This now points to an internal representation declared below (AnimationInternal::InternalAnimationData).
		const void* GetData() const { return m_Data; }

		const Vec3& GetRootTranslationEnd() const { return m_RootTranslationEnd; }
		const Quat& GetRootRotationEnd() const { return m_RootRotationEnd; }

	private:
		void* m_Data;               /// Animation owns this data, and deletes it on destruction
		const Skeleton* m_Skeleton; /// non-owning pointer
		Vec3 m_RootTranslationEnd;
		float m_Duration;
		Quat m_RootRotationEnd;
		uint32_t m_NumTracks;
	};


	/// Animation asset.
	/// Animations ultimately come from a mesh source (so named because it just so happened that meshes were
	/// the first thing that "sources" were used for).   A source is an externally authored digital content file)
	/// One AnimationAsset == one Animation
	/// (so several AnimationAssets might all be referring to the same DCC file (different animations within that file)
	class AnimationAsset : public Asset
	{
	public:
		AnimationAsset(AssetHandle animationSource, AssetHandle mesh, std::string_view animationName, bool extractRootMotion, uint32_t rootBoneIndex, const Vec3 &rootTranslationMask, const Vec3 &rootRotationMask, bool bDiscardRootMotion);

		GLOBAL AssetType GetStaticType() { return AssetType::Animation; }
		virtual ObjectType GetAssetType() const override { return static_cast<ObjectType>(GetStaticType()); }

		AssetHandle GetAnimationSource() const;			/// mesh source (the source asset that contains animation data)
		AssetHandle GetMeshHandle() const;				/// mesh (the mesh that this animation animates (maybe different from the source asset))
		const std::string& GetAnimationName() const;

		/// Parameters controlling extraction of root motion from the specified bone.
		bool IsExtractRootMotion() const;					/// true if we want to extract root motion from the specified bone
		uint32_t RootBoneIndex() const;						/// the index of the bone to extract root motion from
		const Bool3& GetRootTranslationMask() const;	/// mask for translation, true = extract that component, false = do not
		const Bool3& GetRootRotationMask() const;		/// mask for rotation, true = extract that component, false = do not

		/// IsDiscardRootMotion() indicates what should happen with extracted root motion (if any).
		/// True = discard extracted root motion. This converts an animation to "in place".
		/// False = use extracted root motion will be used to create the animation's root motion track.
		/// If IsExtractRootMotion() is false, then IsDiscardRootMotion() has no effect.
		bool IsDiscardRootMotion() const;

		/// Note: can return nullptr (e.g. if dependent assets (e.g. the skeleton source, or the animation source) are deleted _after_ this AnimationAsset has been created.
		const Animation* GetAnimation() const;

		void OnDependencyUpdated(AssetHandle handle);

	private:
		Bool3 m_RootTranslationMask;
		Bool3 m_RootRotationMask;
		AssetHandle m_AnimationSource; /// Animation clips don't necessarily have to come from the same DCC file as the mesh they are animating.
		AssetHandle m_Mesh;            /// For example, the clips could be in one file, and the "skin" (aka, mesh, with skeleton) in a separate file.
		std::string m_AnimationName;   /// The name of the animation within the DCC file (more robust against changes to the DCC than storing the index)
		uint32_t m_RootBoneIndex;
		bool m_IsExtractRootMotion;
		bool m_IsDiscardRootMotion;
	};


	/// This kind of duplicates TransformComponent (see Components.h) and ideally we'd only have one or the other.
	/// However, TransformComponent has a bit more baggage because it needs to support Euler angle rotations.
	/// I don't want to drag that baggage around in the Animation code, hence this slightly smaller and simpler Transform struct.
	/// Transform is also simpler in that it supports only uniform scale, which is going to cover 99% of the use cases for (skeletal) animation.
	///
	/// TODO: Consider switching animation to use rtm::qvsf directly and remove this Transform struct
	struct Transform
	{
	Vec3 Translation = Vec3(0.0f, 0.0f, 0.0f);
		float Scale = 1.0f;
	Quat Rotation = Quat::Identity();

		static const Transform Identity;

		Transform Inverse() const
		{
			Transform result;
			result.Rotation = Rotation.inverse();
			result.Scale = 1.0f / Scale;
			result.Translation = result.Rotation * (result.Scale * -Translation);
			return result;
		}

		Transform& operator*=(const Transform& rhs)
		{
			Translation += Rotation * (Scale * rhs.Translation);
			Rotation *= rhs.Rotation;
			Scale *= rhs.Scale;
			return *this;
		}

		operator Mat4() const
		{
			return Math::ComposeTransform(Translation, Rotation, { Scale, Scale, Scale });
		}
	};


	inline Transform operator*(const Transform& lhs, const Transform& rhs)
	{
		return
		{
		    .Translation = lhs.Translation + lhs.Rotation * (lhs.Scale * rhs.Translation),
		    .Scale = lhs.Scale * rhs.Scale,
		    .Rotation = lhs.Rotation * rhs.Rotation
        };
	}


	struct Pose
	{
		Transform RootMotion;
		float AnimationDuration = 0.0f;
		float AnimationTimePos = 0.0f;
		uint32_t NumBones = 0;

		/// HACK: Animation graph will allocate enough memory for NumBones bone transforms immediately after each instance of struct Pose.
		///       The following gives us access to the bone transforms.
		///
		/// Why don't we put a std::vector<Transform> data member here instead?
		/// Because we need struct Pose to be compatible with choc::value::Value, which does not support dynamically
		/// sized types like std::vector (and we can't use a statically sized std::array<Transform, NumBones> because we
		/// don't know NumBones at compile time).
		const Transform* GetBoneTransforms() const { return reinterpret_cast<const Transform*>(this + 1); }
		Transform* GetBoneTransforms() { return reinterpret_cast<Transform*>(this + 1); }
	};


	namespace Utils::Animation
    {
		Vec3 SafeNormalize(Vec3 v);
		Vec3 TransformVector(const Transform& t, const Vec3& v);
		Vec3 TransformPoint(const Transform& t, const Vec3& v);

		void Retarget(const Transform src[], Transform dest[], const Skeleton& srcSkeletong, const Skeleton& destSkeleton, const std::vector<uint32_t>& boneMap);
	}


}

/// -------------------------------------------------------

/// Internal animation data representation (engine-native, no external deps)
/// Kept in header so importer/creator code can construct it without exposing details elsewhere.
namespace SceneryEditorX::AnimationInternal
{
	struct KeyframeVec3
	{
		float Time = 0.0f;
	Vec3   Value = Vec3(0.0f, 0.0f, 0.0f);
	};

	struct KeyframeQuat
	{
		float     Time = 0.0f;
		Quat Value = Quat::Identity();
	};

	struct TrackTRS
	{
		std::vector<KeyframeVec3> Translations;  /// keyframes for translation
		std::vector<KeyframeQuat> Rotations;     /// keyframes for rotation
		/// Optional: add uniform scale keys if/when needed
	};

	struct InternalAnimationData
	{
		uint32_t NumTracks = 0;
		uint32_t NumFrames = 0;    /// samples per track (for compatibility)
		float    Duration = 0.0f;
		std::vector<TrackTRS> Tracks; /// size == NumTracks
	};
}

/// -------------------------------------------------------
