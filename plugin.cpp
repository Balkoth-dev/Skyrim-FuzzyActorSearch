#include <vector>
#include <rapidfuzz/fuzz.hpp>
#include <string>
#include <optional>

#include "RE/Skyrim.h"

#include "Helpers/StringHelpers.h"
#include <unordered_map>

using namespace stringhelpers;
using namespace rapidfuzz::fuzz;

struct SimpleActorInfo {
    RE::FormID refId;
    RE::FormID baseRefId;
    std::string displayName;
};
struct ActorInfo {
    RE::FormID baseID;     // base FormID (from TESNPC)
    RE::FormID refID;        // reference FormID (from Actor*)
    std::string baseName;    // plugin-defined name (from TESNPC)
    std::string baseNameNorm;  // normalized base name (for searching)
    std::string baseNameDetok;   // detokenized base name (for searching)
    std::string handleName;  // runtime display name (from Actor)
    std::string handleNameNorm;  // normalized handle name (for searching)'
    std::string handleNameDetok;  // detokenized runtime name (for searching)
    double baseScore = 0.0;    // score for base name
    double handleScore = 0.0;           // score for handle name (if any)
    double finalScore = 0.0;   // final score (max of baseScore and handleScore)
};

struct Score {
    double token_set;
    double token_sort;
    double WRatio;
    double partial;
    double simple_ratio;
};

struct QueryTypes {
    std::string qName;
    std::string qNameNorm;
    std::string qNameDetok;
};

std::vector<RE::TESNPC*> g_allActorBases;
std::vector<ActorInfo> g_allActorInfo;

static void CollectAllActorBaseForms() {
    if (!g_allActorBases.empty()) return;  // already collected

    g_allActorBases.clear();

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    // Ensure g_allActorBases is a vector<RE::TESNPC*>
    for (auto* npc : dataHandler->GetFormArray<RE::TESNPC>()) {
        if (npc) {
            g_allActorBases.push_back(npc);
        }
    }

    if (auto* console = RE::ConsoleLog::GetSingleton()) {
        std::string msg = fmt::format("Collected {} actor bases.", g_allActorBases.size());
        console->Print(msg.c_str());
    }
}

std::vector<SimpleActorInfo> GetAllActiveActorsSimple(bool nearbyOnly) {
    auto* console = RE::ConsoleLog::GetSingleton();
    std::vector<SimpleActorInfo> result;
    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) return result;

    // estimate total to reserve
    size_t total = processLists->highActorHandles.size();
    if (!nearbyOnly) {
        total += processLists->middleHighActorHandles.size();
        total += processLists->middleLowActorHandles.size();
        total += processLists->lowActorHandles.size();
    }
    result.reserve(total);

    auto collect = [&](const RE::BSTArray<RE::ActorHandle>& handles) {
        for (const auto& h : handles) {
            if (!h) continue;
            if (auto actorHandle = h.get()) {
                if (auto* actor = actorHandle.get()) {
                    if (auto* base = actor->GetActorBase()) {
                        // copy display name into a stable std::string
                        const char* rawName = actor->GetDisplayFullName();
                        std::string name = rawName ? rawName : std::string{};
                        result.emplace_back(SimpleActorInfo{actor->GetFormID(), base->GetFormID(), std::move(name)});
                    }
                }
            }
        }
    };

    collect(processLists->highActorHandles);
    if (!nearbyOnly) {
        collect(processLists->middleHighActorHandles);
        collect(processLists->middleLowActorHandles);
        collect(processLists->lowActorHandles);
    }

    console->Print(fmt::format("Collected {} active actors.", result.size()).c_str());

    return result;
}

std::vector<ActorInfo> BuildActorInfoList(const std::vector<SimpleActorInfo>& handles, QueryTypes query,
                                          bool handledFilter = false, float a_Weightmin = 50.0) {
    auto* console = RE::ConsoleLog::GetSingleton();

    std::vector<ActorInfo> result;
    result.reserve(g_allActorBases.size());
   
    for (auto* base : g_allActorBases) {
        if (!base) continue;
        ActorInfo info;
        
        auto it = std::find_if(handles.begin(), handles.end(), [&](const SimpleActorInfo& si) {
            return si.baseRefId == base->GetFormID(); });
           
        if (it == handles.end() && handledFilter) {
                continue;  // skip unhandled if requested
        }
        
        info.baseID = base->GetFormID();
        build_norm_and_detok(base->GetName(), info.baseName, info.baseNameNorm, info.baseNameDetok);

        // Calculate score
        double baseScore = (token_set_ratio(query.qNameNorm, info.baseNameNorm) * 0.3) +   // set-based
                           (token_sort_ratio(query.qNameNorm, info.baseNameNorm) * 0.3) +  // sorted tokens
                           (WRatio(query.qNameNorm, info.baseNameNorm) * 0.2) +            // weighted mix
                           (partial_ratio(query.qNameDetok, info.baseNameDetok) * 0.1) +   // substring-friendly
                           (ratio(query.qNameNorm, info.baseNameNorm) * 0.1);

        info.baseScore = std::clamp(baseScore, 0.0, 100.0);              // simple ratio
                                                               
        if (it != handles.end()) {
            info.refID = it->refId;
                build_norm_and_detok(it->displayName, info.handleName, info.handleNameNorm,
                                     info.handleNameDetok);
                        
        // Calculate score
            double handleScore = (token_set_ratio(query.qNameNorm, info.handleNameNorm) * 0.3) +   // set-based
                                 (token_sort_ratio(query.qNameNorm, info.handleNameNorm) * 0.3) +  // sorted tokens
                                 (WRatio(query.qNameNorm, info.handleNameNorm) * 0.2) +            // weighted mix
                                 (partial_ratio(query.qNameDetok, info.handleNameDetok) * 0.1) +   // substring-friendly
                                 (ratio(query.qNameNorm, info.handleNameNorm) * 0.1);

            info.handleScore = std::clamp(handleScore, 0.0, 101.0);  // simple ratio, allow slight edge over base score
        }
        
        info.finalScore = std::max(info.baseScore, info.handleScore);
      //  if (info.finalScore < a_Weightmin) continue;  // skip low-score entries

        result.push_back(std::move(info));

     //  if (info.finalScore >= 100.0) break;  // stop building when perfect match found
    }

        console->Print(fmt::format("Built {} actor list.", result.size()).c_str());

    return result;
}

RE::FormID bk56_SearchActorFuzzy(RE::StaticFunctionTag*, RE::BSFixedString a_name, bool nearbyOnly, bool handledOnly,
                                 float a_weightmin = 50.0) {
    auto console = RE::ConsoleLog::GetSingleton();
    console->Print("[bk56] Starting fuzzy search for: %s", a_name.c_str());
    console->Print("[bk56] NearbyOnly: %s, HandledOnly: %s, WeightMin: %.2f", nearbyOnly ? "true" : "false",
                   handledOnly ? "true" : "false", a_weightmin);

    QueryTypes q;

    build_norm_and_detok(a_name.c_str(), q.qName, q.qNameNorm, q.qNameDetok);
    

    g_allActorInfo.clear();
    auto handles = GetAllActiveActorsSimple(nearbyOnly);
    bool handledFilter = handledOnly || nearbyOnly;
    g_allActorInfo = BuildActorInfoList(handles, q, handledFilter, a_weightmin);
    console->Print("[bk56] Total actors considered: %zu", g_allActorInfo.size());

    std::optional<RE::FormID> bestRefID;

    // Functor for comparison
    struct CompareByValueDesc {
        bool operator()(const ActorInfo& a, const ActorInfo& b) const {
            return a.finalScore > b.finalScore;  // descending
        }
    };
    std::sort(g_allActorInfo.begin(), g_allActorInfo.end(), CompareByValueDesc());

    if (!g_allActorInfo.empty()) {
        ActorInfo& best = g_allActorInfo.front();
        if (best.finalScore >= a_weightmin) {
            bestRefID = best.refID;
            console->Print("[bk56] Final match: Base: %s Handle: %s with FormID %08X with score %.2f", 
                            best.baseName.c_str(), 
                            best.handleName.c_str(), 
                            bestRefID.value_or(0u), best.finalScore);
        } else {
            console->Print("[bk56] No match found above threshold");
        }
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

            //bk56_SearchActorFuzzy(nullptr, "Test", false, 0.0f);  // Test call to verify functionality
        }
    });

    return true;
}