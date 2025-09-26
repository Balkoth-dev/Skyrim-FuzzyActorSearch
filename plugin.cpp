#include <vector>

#include "RE/Skyrim.h"

std::vector<RE::TESObjectREFR*> g_allActorRefs;

// Helper function to collect actor references from a BSTArray<ActorHandle>
void CollectActorRefsFromArray(const RE::BSTArray<RE::ActorHandle>& handles, std::vector<RE::TESObjectREFR*>& out) {
    for (const auto& handle : handles) {
        auto actorPtr = handle.get();
        if (actorPtr) {
            out.push_back(static_cast<RE::TESObjectREFR*>(actorPtr.get()));
        }
    }
}

void CollectAllActiveActorRefs() {
    g_allActorRefs.clear();

    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) return;

    // Collect actor references from each level
    CollectActorRefsFromArray(processLists->highActorHandles, g_allActorRefs);
    CollectActorRefsFromArray(processLists->lowActorHandles, g_allActorRefs);
    CollectActorRefsFromArray(processLists->middleHighActorHandles, g_allActorRefs);
    CollectActorRefsFromArray(processLists->middleLowActorHandles, g_allActorRefs);
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kPostLoadGame ||
            message->type == SKSE::MessagingInterface::kNewGame) {
            CollectAllActiveActorRefs();

            auto* console = RE::ConsoleLog::GetSingleton();
            console->Print("Collected all active actor references.");

            // Print first 100 actors' info
            const size_t maxActors = std::min(static_cast<size_t>(100), g_allActorRefs.size());
            for (size_t i = 0; i < maxActors; ++i) {
                auto* actorRef = g_allActorRefs[i];
                if (actorRef) {
                    // Get base actor to access name
                    auto* baseActor = actorRef->As<RE::Actor>();
                    if (baseActor) {
                        const auto name = baseActor->GetDisplayFullName();
                        const auto refID = actorRef->GetFormID();
                        console->Print(fmt::format("Actor {}: {} (0x{:08X})", i, name, refID).c_str());
                    }
                }
            }

            console->Print(fmt::format("Total actors found: {}", g_allActorRefs.size()).c_str());
        }
    });

    return true;
}
