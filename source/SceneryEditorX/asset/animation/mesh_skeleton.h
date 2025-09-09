/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_skeleton.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <xMath/includes/quat.h>
#include <xMath/includes/vec3.h>
#include "animation.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class MeshSource;

	/// A skeleton is a hierarchy of bones (technically they're actually "joints",
	/// but the term "bone" is widely used for these, so that's what I've gone with).
	/// Each bone has a transform that describes its position relative to its parent.
	/// The bones arranged thus give the "rest pose" of the skeleton.
	class Skeleton
	{
	public:
		static constexpr uint32_t NullIndex = ~0;

		Skeleton() = default;
		Skeleton(uint32_t size);

		bool IsEmpty() const { return m_BoneNames.empty(); }

		uint32_t AddBone(std::string name, uint32_t parentIndex, const Transform& transform);
		uint32_t GetBoneIndex(std::string_view name) const;

		const auto& GetParentBoneIndices() const { return m_ParentBoneIndices; }
		uint32_t GetParentBoneIndex(const uint32_t boneIndex) const { SEDX_CORE_ASSERT(boneIndex < m_ParentBoneIndices.size(), "bone index out of range in Skeleton::GetParentIndex()!"); return m_ParentBoneIndices[boneIndex]; }
		std::vector<uint32_t> GetChildBoneIndexes(uint32_t boneIndex) const;

		uint32_t GetNumBones() const { return static_cast<uint32_t>(m_BoneNames.size()); }
		const std::string& GetBoneName(const uint32_t boneIndex) const { SEDX_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetBoneName()!"); return m_BoneNames[boneIndex]; }
		const auto& GetBoneNames() const { return m_BoneNames; }

		const auto& GetBoneTranslations() const { return m_BoneTranslations; }
		const auto& GetBoneRotations() const { return m_BoneRotations; }
		const auto& GetBoneScales() const { return m_BoneScales; }

		float GetBoneLength(const uint32_t boneIndex) const { SEDX_CORE_ASSERT(boneIndex < m_BoneLengths.size(), "bone index out of range in Skeleton::GetBoneLength()!"); return m_BoneLengths[boneIndex]; }

		/// returns transform for [boneIndex]th bone at rest pose
		/// transform is in bone-space (i.e. relative to parent)
		Transform GetRestPoseTransform(uint32_t boneIndex) const;

		/// returns transform for [boneIndex]th bone at rest pose
		/// transform is in model-space (i.e. relative to root bone)
		const Transform& GetModelSpaceRestPoseTransform(uint32_t boneIndex) const;

		/// returns inverse transform for [boneIndex]th bone at rest pose
		/// transform is in model-space (i.e. relative to root bone)
		const Transform& GetModelSpaceRestPoseInverseTransform(uint32_t boneIndex) const;

		void SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<Vec3> boneTranslations, std::vector<Quat> boneRotations, std::vector<float> boneScales);

		/// Compute various derived data once the skeleton is complete.
		/// Such as:
		/// bone "lengths". The distance from each bone (joint) to the next bone (joint) in the hierarchy.
		/// Leaf bones (joints) are problematic since the raw assets generally don't directly have this length.
		/// For now, they are just set to 1/2 length of parent.
		/// (Other more complicated methods are possible, but involve coupling the skeleton to a skin)
		///
		/// model-space transforms (and their inverses) for each bone at rest pose.
		///
		/// how the skeleton is oriented relative to +Y (can be important for re-targeting)
		void CalculateDerivedData();

		/// Returns the rotation required that would orient the skeleton such that its first bone
		/// (i.e. line from origin to first non-origin joint) is aligned with +Y.
		/// This is important for re-targeting.  Even if both skeletons have the same rest pose, they might not be oriented on the same axis.
		const Quat& GetOrientation() const { return m_Orientation; }

	private:
		Quat m_Orientation = Quat::Identity();
		std::vector<std::string> m_BoneNames;
		std::vector<uint32_t> m_ParentBoneIndices;

		/// rest pose of skeleton. All in bone-local space (i.e. translation/rotation/scale relative to parent)
		std::vector<Vec3> m_BoneTranslations;
		std::vector<Quat> m_BoneRotations;
		std::vector<float> m_BoneScales;

		std::vector<Transform> m_ModelSpaceRestPoseTransforms;          /// model-space transforms for each bone at rest pose
		std::vector<Transform> m_ModelSpaceRestPoseInverseTransforms;   /// inverse of model-space transforms for each bone at rest pose
		std::vector<float> m_BoneLengths;
	};


	/// Basically just an index into a skeleton.
	/// We need a separate type for it though, so when we are instantiating an AnimationGraph we can identify which
	/// node inputs are bone indices (in case we need to do any retargeting).
	struct Bone
	{
		uint32_t Index = Skeleton::NullIndex;
	};


	namespace Utils::Animation
    {
		/// Convert transform for specified bone from bone-space to model-space
		/// boneIndex 0 <=> pose->GetBoneTransforms()[1]   (because 0 is root motion track, not part of skeleton)
		Transform GetModelSpaceBoneTransform(uint32_t boneIndex, const Pose* pose, const Skeleton* skeleton);

		/// Convert pose to model-space matrices in a single pass
		/// Remember: pose->GetBoneTransforms()[0]is the root motion track, which is not part of the skeleton and is not included in the returned transforms.
		std::vector<Transform> GetModelSpaceBoneTransforms(const Pose* pose, const Skeleton* skeleton);

		/// Return a mapping of bone indices from source skeleton to target skeleton.
		/// In other words, target_bone_index = mapping[source_bone_index];
		/// The mapping is determined automatically by comparing bone names.
		std::shared_ptr<std::vector<uint32_t>> GetBoneMap(const Skeleton* source, const Skeleton* target);

		void SetPoseToSkeletonRestPose(Pose* pose, const Skeleton* skeleton);

	}

}

/// -------------------------------------------------------

