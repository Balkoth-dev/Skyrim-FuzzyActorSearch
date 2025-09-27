#include <vector>
#include <rapidfuzz/fuzz.hpp>
#include <string>
#include <optional>

#include "RE/Skyrim.h"

std::vector<RE::TESActorBase*> g_allActorBases;
struct ActorInfo {
    RE::FormID refID;        // reference FormID (from Actor*)
    std::string baseName;    // plugin-defined name (from TESNPC)
    std::string baseNameNorm;  // normalized base name (for searching)
    std::string handleName;  // runtime display name (from Actor)
    std::string handleNameNorm;  // normalized handle name (for searching)
};
std::vector<ActorInfo> g_allActorInfo;



static void CollectAllActorBaseForms() {
    if (g_allActorBases.size() > 0) return;  // already collected
    g_allActorBases.clear();

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    for (auto* npc : dataHandler->GetFormArray<RE::TESNPC>()) {
        if (npc) {
            g_allActorBases.push_back(npc);
        }
    }

    if (auto* console = RE::ConsoleLog::GetSingleton()) {
        console->Print(fmt::format("Collected {} actor bases.", g_allActorBases.size()).c_str());
    }
}



std::vector<RE::ActorHandle> GetAllActiveActorHandles(bool nearbyOnly) {
    std::vector<RE::ActorHandle> result;

    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) return result;

    auto collect = [&](const RE::BSTArray<RE::ActorHandle>& handles) {
        for (auto& h : handles) {
            if (h) result.push_back(h);
        }
    };

    collect(processLists->highActorHandles);
    collect(processLists->middleHighActorHandles);
    if (!nearbyOnly) {
        collect(processLists->lowActorHandles);
        collect(processLists->middleLowActorHandles);
    }
    return result;
}


std::vector<ActorInfo> BuildActorInfoList(const std::vector<RE::ActorHandle>& handles) {
    std::vector<ActorInfo> result;


    for (auto& handle : handles) {
        if (auto actorPtr = handle.get()) {
            if (auto* actor = actorPtr.get()) {
                ActorInfo info{};
                info.refID = actor->GetFormID();

                // Base form (TESNPC)
                if (auto* base = actor->GetActorBase()) {
                    info.baseName = base->GetFullName();
                } else {
                    info.baseName = "<no base>";
                }

                // Runtime name
                info.handleName = actor->GetDisplayFullName();

                result.push_back(std::move(info));
            }
        }
    }

    if (auto* console = RE::ConsoleLog::GetSingleton()) {
        console->Print(fmt::format("Built {} actor list.", result.size()).c_str());
    }

    return result;
}


RE::FormID bk56_SearchActorFuzzy(RE::StaticFunctionTag*, RE::BSFixedString a_name, bool nearbyOnly, float a_weightmin) {
    auto console = RE::ConsoleLog::GetSingleton();
    console->Print("[bk56] Starting fuzzy search for: %s", a_name.c_str());
    console->Print("[bk56] NearbyOnly: %s, WeightMin: %.2f", nearbyOnly ? "true" : "false", a_weightmin);

    g_allActorInfo = BuildActorInfoList(GetAllActiveActorHandles(nearbyOnly));
    console->Print("[bk56] Total actors considered: %zu", g_allActorInfo.size());

    double bestScore = 0.0;
    std::string query = a_name.c_str();
    std::optional<RE::FormID> bestRefID;

    for (const auto& actor : g_allActorInfo) {
        double scoreBase = rapidfuzz::fuzz::WRatio(query, actor.baseName);
        double scoreHandle = rapidfuzz::fuzz::WRatio(query, actor.handleName);
        double chosenScore = std::max(scoreBase, scoreHandle);

        if (chosenScore < a_weightmin) {
       //     console->Print("[bk56] Skipping actor (score below threshold)");
            continue;
        }

        if (chosenScore > bestScore) {
            bestScore = chosenScore;
            bestRefID = actor.refID;
            console->Print("[bk56] New Actor Found: Base %s | Handle %s |Base: %.2f | Handle: %.2f | Chosen: %.2f",
                           actor.baseName.c_str(), actor.handleName.c_str(), scoreBase, scoreHandle, chosenScore);
        }
    }

    if (bestRefID) {
        console->Print("[bk56] Final match: FormID %08X with score %.2f", bestRefID.value(), bestScore);
    } else {
        console->Print("[bk56] No match found above threshold");
    }

    return bestRefID.value_or(0u);
}

bool RegisterFuncs(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("bk56_SearchActorFuzzy", "bk56_SearchActorFuzzy", bk56_SearchActorFuzzy);
    return true;
}


SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
        
    //Etc return stuff here
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kPostLoadGame ||
            message->type == SKSE::MessagingInterface::kNewGame) {
            auto papyrus = SKSE::GetPapyrusInterface();

            auto* console = RE::ConsoleLog::GetSingleton();
            console->Print("[bk56] RegisterFuncs called");
            papyrus->Register(RegisterFuncs);  // This must succeed
            console->Print("[bk56] Registered bk56_SearchActorFuzzy");

            CollectAllActorBaseForms();

            bk56_SearchActorFuzzy(nullptr, "Test", false, 0.0f);  // Test call to verify functionality
        }
    });

    return true;
}