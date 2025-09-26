#include <vector>

#include "RE/Skyrim.h"

std::vector<RE::TESActorBase*> g_allActorBases;
struct ActorInfo {
    RE::FormID refID;        // reference FormID (from Actor*)
    std::string baseName;    // plugin-defined name (from TESNPC)
    std::string handleName;  // runtime display name (from Actor)
};
std::vector<ActorInfo> g_allActorInfo;

static void CollectAllActorBaseForms() {
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



std::vector<RE::ActorHandle> GetAllActiveActorHandles() {
    std::vector<RE::ActorHandle> result;

    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) return result;

    auto collect = [&](const RE::BSTArray<RE::ActorHandle>& handles) {
        for (auto& h : handles) {
            if (h) result.push_back(h);
        }
    };

    collect(processLists->highActorHandles);
    collect(processLists->lowActorHandles);
    collect(processLists->middleHighActorHandles);
    collect(processLists->middleLowActorHandles);

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

    return result;
}


SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kPostLoadGame ||
            message->type == SKSE::MessagingInterface::kNewGame) {

            CollectAllActorBaseForms();
            g_allActorInfo = BuildActorInfoList(GetAllActiveActorHandles());
            for (int i = 0; i < g_allActorInfo.size(); i++) {
                auto& info = g_allActorInfo[i];
                if (auto* console = RE::ConsoleLog::GetSingleton()) {
                    if (info.baseName != info.handleName)
                        console->Print(fmt::format("[{}] RefID: {:08X}, Base: {}, Name: {} <-- MISMATCH", i, info.refID, info.baseName, info.handleName).c_str());
                }
            }
        }
    });

    return true;
}

