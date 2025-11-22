/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2025 Thomas Ray 
 * Copyright (c) 2025 Coalition of Freeware Developers
 * -------------------------------------------------------
 * clear_value.h
 * -------------------------------------------------------
 * Created: 19/11/2025
 * -------------------------------------------------------
 */
#pragma once
#include <vulkan/vulkan.h>
#include <xMath/includes/colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @struct ClearValue
     * @brief Type-safe wrapper for Vulkan clear values with xMath::Color integration
     * 
     * Provides convenient constructors and conversion to VkClearValue for use in
     * render pass operations and explicit clear commands. Integrates seamlessly with
     * xMath's Color class for rich color manipulation and predefined colors.
     * 
     * Key Features:
     *  - Direct integration with xMath::Color for all color operations
     *  - Support for HDR color values (values outside [0.0, 1.0] range)
     *  - Convenient depth/stencil clear value construction
     *  - Automatic conversion to VkClearValue for Vulkan API calls
     *  - Predefined common clear colors via static factory methods
     * 
     * Usage Examples:
     * @code
     * // Using xMath::Color presets
     * ClearValue red = ClearValue::Red();
     * ClearValue cornflowerBlue = ClearValue::CornflowerBlue();
     * 
     * // Using custom xMath::Color
     * xMath::Color customColor(0.5f, 0.3f, 0.8f, 1.0f);
     * ClearValue customClear(customColor);
     * 
     * // Creating from color components
     * ClearValue rgba = ClearValue::FromRGBA(1.0f, 0.5f, 0.0f, 0.8f);
     * 
     * // Depth/stencil clearing
     * ClearValue depth = ClearValue::Depth(1.0f);
     * ClearValue depthStencil = ClearValue::DepthStencil(1.0f, 0);
     * 
     * // HDR color support
     * ClearValue hdrColor = ClearValue::FromRGBA(2.0f, 1.5f, 0.8f, 1.0f);
     * @endcode
     */
    struct ClearValue
    {
        /**
         * @enum Type
         * @brief Discriminator for clear value type (color vs depth/stencil)
         */
        enum class Type : uint8_t
        {
            Color,          ///< Color attachment clear value
            DepthStencil    ///< Depth/stencil attachment clear value
        };

        // Clear value type discriminator
        Type type = Type::Color;

        // Union holding either color or depth/stencil clear data
        union
        {
            VkClearColorValue color;				// Color clear value (RGBA)
            VkClearDepthStencilValue depthStencil;  // Depth/stencil clear value
        };

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Constructors
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Default constructor - creates transparent black (0, 0, 0, 0)
         */
        ClearValue() : color{{0.0f, 0.0f, 0.0f, 0.0f}} {}

        /**
         * @brief Construct from xMath::Color object
         * @param color xMath::Color to convert to clear value
         * 
         * @note - Supports HDR colors (values outside [0.0, 1.0] range)
         * @note - Color components are accessed in RGBA order from xMath::Color union
         * 
         * @code
         * xMath::Color customColor(0.8f, 0.2f, 0.5f, 1.0f);
         * ClearValue clearValue(customColor);
         * 
         * // Using predefined colors
         * ClearValue red(xMath::Color::Red());
         * ClearValue blue(xMath::Color::Blue());
         * @endcode
         */
        explicit ClearValue(const Color& color) : color{{color.r, color.g, color.b, color.a}} {}

        /**
         * @brief Construct from RGBA float components
         * @param r Red component [0.0, 1.0] (HDR values allowed)
         * @param g Green component [0.0, 1.0] (HDR values allowed)
         * @param b Blue component [0.0, 1.0] (HDR values allowed)
         * @param a Alpha component [0.0, 1.0] (HDR values allowed)
         */
        explicit ClearValue(float r, float g, float b, float a = 1.0f) : color{{r, g, b, a}} {}

        /**
         * @brief Construct from Vec4 color vector
         * @param colorVec Vector with x=red, y=green, z=blue, w=alpha
         */
        explicit ClearValue(const Vec4& colorVec) : color{{colorVec.x, colorVec.y, colorVec.z, colorVec.w}} {}

        /**
         * @brief Construct depth/stencil clear value
         * @param depth Depth clear value [0.0, 1.0] (typically 1.0 for far plane)
         * @param stencil Stencil clear value (typically 0)
         */
        explicit ClearValue(float depth, uint32_t stencil = 0) : type(Type::DepthStencil), depthStencil{depth, stencil} {}

        // Dark gray (0.1, 0.1, 0.1, 1) - used for editor viewports
        static ClearValue DarkGray() { return ClearValue(0.1f, 0.1f, 0.1f, 1.0f); }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Depth/Stencil Clear Values
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Create depth clear value with default far plane (1.0)
         * @param depth Depth value [0.0, 1.0] (default: 1.0 = far plane)
         * @return Depth clear value with stencil=0
         */
        static ClearValue Depth(float depth = 1.0f) { return ClearValue(depth, 0); }

        /**
         * @brief Create depth/stencil clear value
         * @param depth Depth value [0.0, 1.0]
         * @param stencil Stencil value
         * @return Depth/stencil clear value
         */
        static ClearValue DepthStencil(float depth, uint32_t stencil) { return ClearValue(depth, stencil); }

        /**
         * @brief Default depth clear value (1.0 depth, 0 stencil)
         * @return Depth/stencil clear value for far plane
         */
        static ClearValue DefaultDepth() { return ClearValue(1.0f, 0); }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Vulkan Conversion
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Convert to VkClearValue for Vulkan API calls
         * @return VkClearValue with appropriate color or depth/stencil data
         * 
         * @code
         * ClearValue clearColor = ClearValue::Red();
         * VkClearValue vkClear = static_cast<VkClearValue>(clearColor);
         * 
         * // Or implicitly
         * std::array<VkClearValue, 2> clearValues{};
         * clearValues[0] = ClearValue::CornflowerBlue();
         * clearValues[1] = ClearValue::DefaultDepth();
         * @endcode
         */
        explicit operator VkClearValue() const
        {
            VkClearValue vkClearValue;
            if (type == Type::Color)
                vkClearValue.color = color;
            else
                vkClearValue.depthStencil = depthStencil;
            return vkClearValue;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Utility Methods
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Check if this is a color clear value
         * @return True if color, false if depth/stencil
         */
        [[nodiscard]] bool IsColor() const noexcept { return type == Type::Color; }

        /**
         * @brief Check if this is a depth/stencil clear value
         * @return True if depth/stencil, false if color
         */
        [[nodiscard]] bool IsDepthStencil() const noexcept { return type == Type::DepthStencil; }

        /**
         * @brief Convert color clear value to xMath::Color
         * @return xMath::Color representation
         * @throws std::runtime_error if called on depth/stencil clear value
         */
        [[nodiscard]] Color ToColor() const
        {
            if (type != Type::Color)
                throw std::runtime_error("Cannot convert depth/stencil clear value to Color");

            return {color.float32[0], color.float32[1], color.float32[2], color.float32[3]};
        }

        /**
         * @brief Create a new clear value with modified alpha
         * @param alpha New alpha value [0.0, 1.0]
         * @return New color clear value with modified alpha
         * @throws std::runtime_error if called on depth/stencil clear value
         */
        [[nodiscard]] ClearValue WithAlpha(float alpha) const
        {
            if (type != Type::Color)
                throw std::runtime_error("Cannot modify alpha on depth/stencil clear value");

            return ClearValue(color.float32[0], color.float32[1], color.float32[2], alpha);
        }
    };

} // namespace SceneryEditorX

// -------------------------------------------------------
