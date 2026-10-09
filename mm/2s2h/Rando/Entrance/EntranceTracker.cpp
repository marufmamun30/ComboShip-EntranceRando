#include "EntranceTracker.h"
#include "Entrance.h"

#include "2s2h/BenGui/UIWidgets.hpp"
#include <libultraship/bridge/consolevariablebridge.h>

#include <algorithm>
#include <imgui.h>
#include <string>
#include <vector>

namespace Rando {

namespace Entrance {

#define CVAR_SHOW_UNDISCOVERED "gRando.EntranceTracker.ShowUndiscovered"
#define CVAR_REVEAL_UNDISCOVERED "gRando.EntranceTracker.RevealUndiscovered"
#define CVAR_GROUP_BY_DESTINATION "gRando.EntranceTracker.GroupByDestination"
#define CVAR_SORT_ALPHABETICAL "gRando.EntranceTracker.SortAlphabetical"
#define CVAR_COLLAPSE_UNDISCOVERED "gRando.EntranceTracker.CollapseFound"

static const ImVec4 COLOR_DISCOVERED = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 COLOR_UNDISCOVERED = ImVec4(1.0f, 1.0f, 1.0f, 0.45f);
static const ImVec4 COLOR_LAST_TAKEN = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
static const ImVec4 COLOR_ARROW = ImVec4(1.0f, 0.85f, 0.3f, 1.0f);

struct TrackerRow {
    s16 entrance;
    s16 connection;
    std::string source;
    std::string destination;
    bool discovered;
};

void EntranceTrackerWindow::DrawElement() {
    if (!IS_RANDO || !IsShuffled()) {
        ImGui::TextWrapped("Entrances are not shuffled in this save.");
        return;
    }

    static ImGuiTextFilter filter;
    UIWidgets::PushStyleCombobox(UIWidgets::Colors::Gray);
    filter.Draw("##entranceTrackerSearch", ImGui::GetContentRegionAvail().x);
    UIWidgets::PopStyleCombobox();
    if (!filter.IsActive()) {
        ImGui::SameLine(18.0f);
        ImGui::TextColored(COLOR_UNDISCOVERED, "Search");
    }

    if (ImGui::CollapsingHeader("Options")) {
        UIWidgets::CVarCheckbox("Show Undiscovered Entrances", CVAR_SHOW_UNDISCOVERED,
                                UIWidgets::CheckboxOptions({ { .tooltip = "List entrances you have not been "
                                                                          "through yet." } })
                                    .DefaultValue(true));
        UIWidgets::CVarCheckbox(
            "Reveal Undiscovered Destinations", CVAR_REVEAL_UNDISCOVERED,
            UIWidgets::CheckboxOptions(
                { { .tooltip = "Show where an entrance leads before you have been through it. This is a spoiler." } }));
        UIWidgets::CVarCheckbox(
            "Group by Destination", CVAR_GROUP_BY_DESTINATION,
            UIWidgets::CheckboxOptions(
                { { .tooltip = "Group entrances by the area they lead to instead of the area they are found in." } }));
        UIWidgets::CVarCheckbox("Sort Alphabetically", CVAR_SORT_ALPHABETICAL,
                                UIWidgets::CheckboxOptions({ { .tooltip = "Otherwise entrances are listed in the "
                                                                          "order the game defines them." } })
                                    .DefaultValue(true));
    }

    bool showUndiscovered = CVarGetInteger(CVAR_SHOW_UNDISCOVERED, 1);
    bool revealUndiscovered = CVarGetInteger(CVAR_REVEAL_UNDISCOVERED, 0);
    bool groupByDestination = CVarGetInteger(CVAR_GROUP_BY_DESTINATION, 0);
    bool sortAlphabetical = CVarGetInteger(CVAR_SORT_ALPHABETICAL, 1);

    auto& entrances = GetEntrances();
    s16 lastTaken = GetLastEntranceTaken();

    std::vector<TrackerRow> groups[EG_MAX];
    int total = 0;
    int found = 0;
    for (size_t i = 0; i < entrances.size(); i++) {
        s16 connection = GetConnection((s16)i);
        if (connection < 0 || connection == (s16)i) {
            continue;
        }
        bool discovered = IsDiscovered((s16)i);
        total++;
        found += discovered ? 1 : 0;
        if (!discovered && !showUndiscovered) {
            continue;
        }

        TrackerRow row = { (s16)i, connection, entrances[i].source + " to " + entrances[i].destination,
                           GetArrivalName(connection), discovered };
        bool known = discovered || revealUndiscovered;
        if (filter.IsActive() && !filter.PassFilter(row.source.c_str()) &&
            !(known && filter.PassFilter(row.destination.c_str()))) {
            continue;
        }
        EntranceGroup group = groupByDestination && known ? entrances[connection].group : entrances[i].group;
        groups[group].push_back(row);
    }

    ImGui::Text("Discovered: %d / %d", found, total);
    ImGui::Separator();

    ImGui::BeginChild("entranceTrackerList", ImVec2(0, 0));
    for (int group = 0; group < EG_MAX; group++) {
        auto& rows = groups[group];
        if (rows.empty()) {
            continue;
        }
        if (sortAlphabetical) {
            std::sort(rows.begin(), rows.end(),
                      [](const TrackerRow& a, const TrackerRow& b) { return a.source < b.source; });
        }

        int groupFound = 0;
        for (auto& row : rows) {
            groupFound += row.discovered ? 1 : 0;
        }
        std::string header = std::string(GetGroupName((EntranceGroup)group)) + " (" + std::to_string(groupFound) + "/" +
                             std::to_string(rows.size()) + ")###entranceGroup" + std::to_string(group);
        if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }

        for (auto& row : rows) {
            ImVec4 color =
                row.entrance == lastTaken ? COLOR_LAST_TAKEN : (row.discovered ? COLOR_DISCOVERED : COLOR_UNDISCOVERED);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("%s", row.source.c_str());
            ImGui::PopStyleColor();

            ImGui::Indent(16.0f);
            ImGui::TextColored(COLOR_ARROW, "->");
            ImGui::SameLine();
            if (row.discovered || revealUndiscovered) {
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextWrapped("%s", row.destination.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextColored(COLOR_UNDISCOVERED, "???");
            }
            ImGui::Unindent(16.0f);
        }
    }
    ImGui::EndChild();
}

} // namespace Entrance

} // namespace Rando
