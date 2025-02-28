/*
 * Credits: silviu20092
 */

#include "ScriptMgr.h"
#include "Config.h"
#include "improved_bank.h"

class improved_bank_worldscript : public WorldScript
{
public:
    improved_bank_worldscript() : WorldScript("improved_bank_worldscript")
    {
    }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sImprovedBank->SetAccountWide(sConfigMgr->GetOption<bool>("ImprovedBank.AccountWide", true));
        sImprovedBank->SetSearchBank(sConfigMgr->GetOption<bool>("ImprovedBank.Deposit.SearchBank", false));
        sImprovedBank->SetShowDepositReagents(sConfigMgr->GetOption<bool>("ImprovedBank.Deposit.AllReagents", true));
        sImprovedBank->SetDepositReagentsSearchBank(sConfigMgr->GetOption<bool>("ImprovedBank.Deposit.AllReagents.Bank", false));
        sImprovedBank->SetBlacklistedSubclasses(sConfigMgr->GetOption<std::string>("ImprovedBank.Deposit.AllReagents.BlacklistSubclass", "-1"));
    }
};

void AddSC_improved_bank_worldscript()
{
    new improved_bank_worldscript();
}
