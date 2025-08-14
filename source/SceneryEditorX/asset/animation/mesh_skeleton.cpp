/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_skeleton.cpp
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#include <algorithm>
#include <SceneryEditorX/asset/animation/mesh_skeleton.h>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/utils/string_utils.h>
#include <SceneryEditorX/utils/math/math_utils.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	namespace Utils::Animation
    {

		Transform GetModelSpaceBoneTransform(const uint32_t boneIndex, const Pose* pose, const Skeleton* skeleton)
		{
			SEDX_CORE_ASSERT(boneIndex < pose->NumBones - 1, "bone index out of range in GetModelSpaceBoneTransform()!");
			const Transform* boneTransforms = pose->GetBoneTransforms();
			Transform modelSpaceTransform = boneTransforms[boneIndex + 1];
			uint32_t parentBoneIndex = skeleton->GetParentBoneIndex(boneIndex);
			while (parentBoneIndex != Skeleton::NullIndex)
			{
				modelSpaceTransform = boneTransforms[parentBoneIndex + 1] * modelSpaceTransform;
				parentBoneIndex = skeleton->GetParentBoneIndex(parentBoneIndex);
			}

			return modelSpaceTransform;
		}


		std::vector<Transform> GetModelSpaceBoneTransforms(const Pose* pose, const Skeleton* skeleton)
		{
			uint32_t N = pose->NumBones;
			std::vector<Transform> modelSpaceTransforms(N - 1);
			const Transform* boneTransforms = pose->GetBoneTransforms();
			modelSpaceTransforms[0] = boneTransforms[1];
			for (uint32_t i = 2; i < N; ++i)
			{
				uint32_t boneIndex = i - 1;
				modelSpaceTransforms[boneIndex] = modelSpaceTransforms[skeleton->GetParentBoneIndex(boneIndex)] * boneTransforms[i];
			}
			return modelSpaceTransforms;
		}


		std::string RemoveKnownPrefixes(const std::string& boneName)
		{
			std::string normalizedBoneName = boneName;
			if (auto colon = normalizedBoneName.find_last_of(':'); colon != std::string::npos)
			{
				normalizedBoneName = normalizedBoneName.substr(colon + 1);
			}

			/// strip common "def-" prefix.  (we don't strip "mch-", "ctl-", or "org-" as generally we want to match to the "def-" bones)
			/// strip known vrm prefixes
			for (std::string knownPrefix : {"def-", "bip_c_", "bip_", "sec_", "adj_"})
			{
				/// if bone name begins with known prefix, strip it
				if (normalizedBoneName.starts_with(knownPrefix))
				{
					normalizedBoneName.erase(0, knownPrefix.length());
					break; /// only strip the first known prefix
				}
			}

			return normalizedBoneName;
		}


		/// Search boneNames to see if there is a common prefix to all names.
		/// If so, remove it.
		/// (e.g. "mixamorig:",  "name-of-model-here" etc.)
		void RemovePrefixes(std::vector<std::string>& boneNames)
		{
			if (boneNames.size() > 1)
			{
				std::string commonPrefix = boneNames[0];
				if (commonPrefix == "root")
				{
					commonPrefix = boneNames[1];
				}

				for (const auto& name : boneNames)
				{
					if (name == "root")
                        continue;

                    size_t i = 0;
					while (i < commonPrefix.size() && i < name.size() && commonPrefix[i] == name[i])
					{
						++i;
					}
					commonPrefix = commonPrefix.substr(0, i);
				}

				if (!commonPrefix.empty())
				{
					for (auto& name : boneNames)
					{
						if (name == "root")
                            continue;

                        name.erase(0, commonPrefix.size());
					}
				}
			}

			/// remove known prefixes not dealt with by above (e.g. some bones may have different prefix e.g. DEF-, CTL-,  BIP-, SEC- etc.)
            std::ranges::transform(boneNames, boneNames.begin(), RemoveKnownPrefixes);
		}


		/// Normalizes the bone name to a canonical form.  This is used to match bones between different skeletons.
		/// The canonical form is lower case, with left/right prefixes or suffixes replaced with .l or .r.
		std::string NormalizedBoneName(std::string_view boneName)
		{
			std::string normalizedBoneName{ boneName };

			/// replace any combo of left/right prefixed or suffixed with any of underscore, hyphen, space, or dot with .l or .r
			/// with .l or .r suffix.
			/// e.g. "left_foot" -> "foot.l", "foot_left" -> "foot.l", "left foot" -> "foot.l", "foot left" -> "foot.l",
			bool done = false;
			for (std::string side : { "left", "right" })
			{
				for(std::string extra : { "_", "-", " ", ".", "" })
				{
					std::string searchString = extra + side;
					if (auto pos = normalizedBoneName.find(searchString); pos != std::string::npos)
					{
						normalizedBoneName.erase(pos, searchString.length());
						normalizedBoneName += ".";
						normalizedBoneName += side[0];
						done = true;
						break;
					}

					searchString = side + extra;
					if (auto pos = normalizedBoneName.find(searchString); pos != std::string::npos)
					{
						normalizedBoneName.erase(pos, searchString.length());
						normalizedBoneName += ".";
						normalizedBoneName += side[0];
						done = true;
						break;
					}
				}
				if (done)
                    break;
            }

			/// replace leading l- or r- (or trailing -l or -r) with .l or .r
			/// must be the first (or last) 2 characters
			done = false;
			if (normalizedBoneName.length() > 2)
			{
				for (std::string side : {"l", "r"})
				{
					for (std::string extra : {"-", "_", " "})
					{
						std::string searchString = side + extra;
						if (normalizedBoneName.substr(0, 2) == searchString)
						{
							normalizedBoneName.erase(0, 2);
							normalizedBoneName += "." + side;
							done = true;
							break;
						}

						searchString = extra + side;
						if (normalizedBoneName.substr(normalizedBoneName.length() - 2) == searchString)
						{
							normalizedBoneName.erase(normalizedBoneName.length() - 2);
							normalizedBoneName += "." + side;
							done = true;
							break;
						}
					}
					if (done)
                        break;
                }
			}

			/// replace synonyms
			/// Upper arm, lower arm and synonyms are all mapped to "arm"
			/// Upper leg lower leg and synonyms are all mapped to "leg"
			/// We sort out which are the upper/lower later.
			///
			/// ofc, this is never going to be perfect.  Can add to it as we encounter more cases.
			static std::map<std::string, std::vector<std::string>> allSynonyms =
			{
				{ "hips", { "pelvis" } },
				{ "spine", { "torso", "upperchest", "chest", "abdomen", "stomach" } },
				{ "shoulder", { "clavicle", "clav" } },
				{ "arm", { "upper_arm", "upper-arm", "upperarm", "up_arm", "up-arm", "uparm", "lower_arm", "lower-arm", "lowerarm", "low_arm", "low-arm", "lowarm", "elbow", "forearm" } },
				{ "leg", { "thigh", "upper_leg", "upper-leg", "upperleg", "up_leg", "up-leg", "upleg", "lower_leg", "lower-leg", "lowerleg", "low_leg", "low-leg", "lowleg", "knee", "shin", "calf" }},
				{ "foot", { "ankle", "heel" } },
				{ "toe", { "ball", "toes", "toebase" } },
				{ "hand", { "wrist", "palm" } },
				{ "thumb", { "finger0" "finger-0", "finger_0", "handthumb" } },
				{ "index", { "finger1", "finger-1", "finger_1", "handindex", "f_index", "finger_index", "finger-index", "fingerindex", "indexfinger", "index-finger", "index_finger", "index-f", "index_f" } },
				{ "middle", { "finger2", "finger-2", "finger_2", "handmiddle", "f_middle", "finger_middle", "finger-middle", "fingermiddle", "middlefinger", "middle-finger", "middle_finger", "middle-f", "middle_f" } },
				{ "ring", { "finger3", "finger-3", "finger_3", "handring", "f_ring", "finger_ring", "finger-ring", "fingerring", "ringfinger", "ring-finger", "ring_finger", "ring-f", "ring_f"  } },
				{ "pinky", { "finger4", "finger-4", "finger_4", "little", "handpinky", "handlittle", "f_pinky", "finger_pinky", "finger-pinky", "fingerpinky", "pinkyfinger", "pinky-finger", "pinky_finger", "pinky-f", "pinky_f"  } }
			};

			done = false;
			for (const auto& [name, synonyms] : allSynonyms)
			{
				for (const auto& synonym : synonyms)
				{
					if (auto pos = normalizedBoneName.find(synonym); pos != std::string::npos)
					{
						normalizedBoneName.replace(pos, synonym.length(), name);
						done = true;
						break;
					}
				}
				if (done)
                    break;
            }

			/// special case. hip -> hips
			if(normalizedBoneName == "hip")
                normalizedBoneName = "hips";

            /// Strip off any numeric suffixes  spine1, spine2 -> spine,   spine.001, spine.002 -> spine  etc.
			/// Ignore .l and .r suffixes

			/// if normalizedBoneName ends with .l or .r, make a temporary copy without the suffix
			/// and then strip the numeric suffixes from that.
			/// then if the original name ends with .l or .r, append it back on.
			std::string tempName = normalizedBoneName;
			if (normalizedBoneName.length() > 2 && (normalizedBoneName.substr(normalizedBoneName.length() - 2) == ".l" || normalizedBoneName.substr(normalizedBoneName.length() - 2) == ".r"))
                tempName = normalizedBoneName.substr(0, normalizedBoneName.length() - 2);

            if (auto pos = tempName.find_first_of("0123456789"); pos != std::string::npos)
			{
                if (std::string suffix = tempName.substr(pos); std::ranges::all_of(suffix, isdigit))
                    tempName.erase(pos);
            }

			for(std::string separator : {".", "_", "-", " "})
			{
				if (auto pos = tempName.find_last_of(separator); pos != std::string::npos)
				{
                    if (std::string suffix = tempName.substr(pos + 1); std::ranges::all_of(suffix, isdigit))
                        tempName.erase(pos);

                    break;
				}
			}

			if (normalizedBoneName.length() > 2 && (normalizedBoneName.substr(normalizedBoneName.length() - 2) == ".l" || normalizedBoneName.substr(normalizedBoneName.length() - 2) == ".r"))
                normalizedBoneName = tempName + normalizedBoneName.substr(normalizedBoneName.length() - 2);
            else
                normalizedBoneName = tempName;

            return normalizedBoneName;
		}

		/// Renumber sequences of same-named bones.  spine, spine, spine -> spine.1, spine.2, spine.3 etc.
		void RenumberBoneChains(std::vector<std::string>& boneNames)
		{
			static std::set<std::string> chains = {
			    "spine",
			    "arm.l","arm.r",
			    "leg.l","leg.r",
			    "thumb.l","thumb.r",
			    "index.l","index.r",
			    "middle.l","middle.r",
			    "ring.l","ring.r",
			    "pinky.l","pinky.r"
			};

			for (const auto& chain : chains)
			{
				for (auto i = 0; i < boneNames.size(); ++i)
				{
					if (boneNames[i].find(chain) != std::string::npos)
					{
						uint32_t k = 0;
						for (auto j = i; j < boneNames.size(); ++j)
						{
							if (boneNames[j].find(chain) != std::string::npos)
                                boneNames[j] += "." + std::to_string(k++);
                        }
						break;
					}
				}
			}
		}


		void NormalizeBoneNames(std::vector<std::string>& boneNames)
		{
			std::ranges::transform(boneNames, boneNames.begin(), String::ToLower);
			RemovePrefixes(boneNames);
            std::ranges::transform(boneNames, boneNames.begin(), NormalizedBoneName);
			RenumberBoneChains(boneNames);
		}

		std::shared_ptr<std::vector<uint32_t>> GetBoneMap(const Skeleton* source, const Skeleton* target)
		{
			static std::map<std::pair<const Skeleton*, const Skeleton*>, std::shared_ptr<std::vector<uint32_t>>> cache;

			SEDX_CORE_ASSERT(source, "source skeleton is null in GetBoneMap()!");
			SEDX_CORE_ASSERT(target, "target skeleton is null in GetBoneMap()!");

			if(auto cachedMap = cache.find({ source, target }); cachedMap != cache.end())
                return cachedMap->second;

            auto sourceBones = source->GetBoneNames();
			auto targetBones = target->GetBoneNames();

			NormalizeBoneNames(sourceBones);
			NormalizeBoneNames(targetBones);

			auto map = std::make_shared<std::vector<uint32_t>>(source->GetNumBones(), Skeleton::NullIndex);
			for (uint32_t i = 0; i < source->GetNumBones(); ++i)
			{
				const auto& name1 = sourceBones[i];

				if (name1 == "root")
				{
					/// don't remap root bone.
					/// this bone is generally used for axis orientation / scale rather than being part of the animation
					continue;
				}

				for (uint32_t j = 0; j < target->GetNumBones(); ++j)
				{
					if (name1 == targetBones[j])
					{
						map->at(i) = j;
						break;
					}
				}
			}
			cache[{source, target}] = map;

			return cache[{source, target}];
		}

		void SetPoseToSkeletonRestPose(Pose* pose, const Skeleton* skeleton)
		{
			SEDX_CORE_ASSERT(pose, "pose is null in SetPoseToSkeletonRestPose()!");
			SEDX_CORE_ASSERT(skeleton, "skeleton is null in SetPoseToSkeletonRestPose()!");
			SEDX_CORE_ASSERT(pose->NumBones == skeleton->GetNumBones() + 1, "Pose and Skeleton have different number of bones in SetPoseToSkeletonRestPose()!");

			auto* poseBoneTransforms = pose->GetBoneTransforms();
			for (uint32_t i = 0; i < skeleton->GetNumBones(); ++i)
                poseBoneTransforms[i + 1] = skeleton->GetRestPoseTransform(i);
        }

	}


	Skeleton::Skeleton(uint32_t size)
	{
		m_BoneNames.reserve(size);
		m_ParentBoneIndices.reserve(size);
		m_BoneTranslations.reserve(size);
		m_BoneRotations.reserve(size);
		m_BoneScales.reserve(size);
		m_BoneLengths.reserve(size);
	}


	uint32_t Skeleton::AddBone(std::string name, uint32_t parentIndex, const Transform& transform)
	{
		uint32_t index = static_cast<uint32_t>(m_BoneNames.size());
		m_BoneNames.emplace_back(name);
		m_ParentBoneIndices.emplace_back(parentIndex);
		m_BoneTranslations.emplace_back(transform.Translation);
		m_BoneRotations.emplace_back(transform.Rotation);
		m_BoneScales.emplace_back(transform.Scale);
		return index;
	}


	uint32_t Skeleton::GetBoneIndex(const std::string_view name) const
	{
		for (size_t i = 0; i < m_BoneNames.size(); ++i)
		{
			if (m_BoneNames[i] == name)
                return static_cast<uint32_t>(i);
        }

		return Skeleton::NullIndex;
	}


	std::vector<uint32_t> Skeleton::GetChildBoneIndexes(const uint32_t boneIndex) const
	{
		std::vector<uint32_t> childBoneIndexes;
		for (size_t i = 0; i < m_ParentBoneIndices.size(); ++i)
		{
			if (m_ParentBoneIndices[i] == boneIndex)
                childBoneIndexes.emplace_back(static_cast<uint32_t>(i));
        }

		return childBoneIndexes;
	}


	Transform Skeleton::GetRestPoseTransform(const uint32_t boneIndex) const
	{
		SEDX_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetRestPoseTransform()!");
		return {
		    .Translation = m_BoneTranslations[boneIndex],
		    .Scale = m_BoneScales[boneIndex],
		    .Rotation = m_BoneRotations[boneIndex]
		};
	}

	const Transform& Skeleton::GetModelSpaceRestPoseTransform(const uint32_t boneIndex) const
	{
		SEDX_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetModelSpaceRestPoseTransform()!");
		return m_ModelSpaceRestPoseTransforms[boneIndex];
	}

	const Transform& Skeleton::GetModelSpaceRestPoseInverseTransform(const uint32_t boneIndex) const
	{
		SEDX_CORE_ASSERT(boneIndex < m_BoneNames.size(), "bone index out of range in Skeleton::GetModelSpaceRestPoseTransform()!");
		return m_ModelSpaceRestPoseInverseTransforms[boneIndex];
	}

	void Skeleton::SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<Vec3> boneTranslations, std::vector<Quat> boneRotations, std::vector<float> boneScales)
	{
		SEDX_CORE_ASSERT(parentBoneIndices.size() == boneNames.size());
		SEDX_CORE_ASSERT(boneTranslations.size()  == boneNames.size());
		SEDX_CORE_ASSERT(boneRotations.size()     == boneNames.size());
		SEDX_CORE_ASSERT(boneScales.size()        == boneNames.size());
		m_BoneNames = std::move(boneNames);
		m_ParentBoneIndices = std::move(parentBoneIndices);
		m_BoneTranslations = std::move(boneTranslations);
		m_BoneRotations = std::move(boneRotations);
		m_BoneScales = std::move(boneScales);

		CalculateDerivedData();
	}

	void Skeleton::CalculateDerivedData()
	{
		uint32_t N = GetNumBones();

		/// work out model-space transforms
		m_ModelSpaceRestPoseTransforms.resize(N);
		m_ModelSpaceRestPoseInverseTransforms.resize(N);
		m_ModelSpaceRestPoseTransforms[0] = GetRestPoseTransform(0);
		m_ModelSpaceRestPoseInverseTransforms[0] = m_ModelSpaceRestPoseTransforms[0].Inverse();
		for (uint32_t i = 1; i < N; ++i)
		{
			m_ModelSpaceRestPoseTransforms[i] = m_ModelSpaceRestPoseTransforms[m_ParentBoneIndices[i]] * GetRestPoseTransform(i);
			m_ModelSpaceRestPoseInverseTransforms[i] = m_ModelSpaceRestPoseTransforms[i].Inverse();
		}

		m_BoneLengths.resize(N);
		for (uint32_t i = 0; i < N; i++)
		{
			auto boneTransform = GetModelSpaceRestPoseTransform(i);
			auto children = GetChildBoneIndexes(i);

			float length = 0.25f;
			if (children.empty())
			{
				/// set leaf bone length to 1/2 scale of parent, if any
				if (auto parent = GetParentBoneIndex(i); parent != Skeleton::NullIndex)
                    length = GetBoneLength(parent) * 0.5f;
            }
			else
			{
				const auto& firstChildBoneTransform = GetModelSpaceRestPoseTransform(children[0]);
				Vec3 direction = firstChildBoneTransform.Translation - boneTransform.Translation;
				// Use native utility instead of glm::length
				length = SceneryEditorX::Utils::Distance(boneTransform.Translation, firstChildBoneTransform.Translation);
			}

			m_BoneLengths[i] = length;
		}

		/// Guess how the skeleton is oriented (relative to +Y up)
		/// by looking at position of first non-origin bone.
		/// Returns the rotation quaternion required that would point the skeleton towards +Y
		for (uint32_t i = 0; i < N; ++i)
		{
			if (auto direction = GetModelSpaceRestPoseTransform(i).Translation; direction  != Vec3(0.0f, 0.0f, 0.0f))
			{
				/// (probably) only need the maximum component of the direction vector
				/// to determine which axis the skeleton is aligned to
				if (std::abs(direction.x) >= std::abs(direction.y) && std::abs(direction.x) >= std::abs(direction.z))
				{
					direction.y = 0.0f;
					direction.z = 0.0f;
				}
				else if (std::abs(direction.y) >= std::abs(direction.x) && std::abs(direction.y) >= std::abs(direction.z))
				{
					direction.x = 0.0f;
					direction.z = 0.0f;
				}
				else
				{
					direction.x = 0.0f;
					direction.y = 0.0f;
				}

				// Rotate the skeleton's dominant axis direction to +Y using native quaternion helper
				m_Orientation = Quat::FromToRotation(SceneryEditorX::Utils::Normalize(direction), Vec3(0.0f, 1.0f, 0.0f));
				break;
			}
		}
	}


}

/// -------------------------------------------------------
