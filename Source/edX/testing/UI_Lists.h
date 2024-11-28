#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vector>
#include <string>

#include "../edXProjectFile.h"

struct AssetData
{
    char assetName[128] = "Asset0001";
    double latitude = 999999.00000001;
    double longitude = 999999.00000001;
    float heading = 0.0f;
    int altitude = 123;
    int assetType = 0;
    bool locked = false;
    bool hidden = false;
};

std::vector<AssetData> assetDataList;

void FileListsStack()
{
    ImGui::BeginChild("AssetDataList", ImVec2(1180, 830), true);

    if (ImGui::BeginTable("table1", 8,
        ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchSame
    ))
    {
        // Table headers
        ImGui::TableSetupColumn("Asset Name");
        ImGui::TableSetupColumn("Latitude");
        ImGui::TableSetupColumn("Longitude");
        ImGui::TableSetupColumn("Heading");
        ImGui::TableSetupColumn("Altitude");
        ImGui::TableSetupColumn("Asset Type");
        ImGui::TableSetupColumn("Locked");
        ImGui::TableSetupColumn("Hidden");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < assetDataList.size(); ++i)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::PushItemWidth(200);
            ImGui::InputText(("##AssetName" + std::to_string(i)).c_str(),
                             assetDataList[i].assetName,
                             IM_ARRAYSIZE(assetDataList[i].assetName));
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(290);
            ImGui::InputDouble(("##Latitude" + std::to_string(i)).c_str(),
                               &assetDataList[i].latitude,
                               0.01f,
                               1.0f,
                               "%.8f");
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(2);
            ImGui::PushItemWidth(290);
            ImGui::InputDouble(("##Longitude" + std::to_string(i)).c_str(),
                               &assetDataList[i].longitude,
                               0.01f,
                               1.0f,
                               "%.8f");
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(3);
            ImGui::PushItemWidth(100);
            ImGui::SliderAngle(("##Heading" + std::to_string(i)).c_str(), &assetDataList[i].heading);
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(4);
            ImGui::PushItemWidth(120);
            ImGui::InputInt(("##Altitude" + std::to_string(i)).c_str(), &assetDataList[i].altitude);
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(5);
            ImGui::PushItemWidth(140);
            const char *items[] = {"Object", "Runway", "Windsock", "Light", "Tower"};
            ImGui::Combo(("##AssetType" + std::to_string(i)).c_str(),
                         &assetDataList[i].assetType,
                         items,
                         IM_ARRAYSIZE(items));
            ImGui::PopItemWidth();

            ImGui::TableSetColumnIndex(6);
            ImGui::PushItemWidth(21);
            ImGui::Checkbox("##Locked", &assetDataList[i].locked);

            ImGui::TableSetColumnIndex(7);
            ImGui::PushItemWidth(21);
            ImGui::Checkbox("##Hidden", &assetDataList[i].hidden);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}
