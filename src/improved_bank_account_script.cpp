/*
 * Credits: silviu20092
 */

#include "ScriptMgr.h"
#include "DatabaseEnv.h"

class improved_bank_account_script : public AccountScript
{
public:
    improved_bank_account_script() : AccountScript("improved_bank_account_script") { }

    void OnBeforeAccountDelete(uint32 accountId)
    {
        CharacterDatabase.Execute("DELETE FROM mod_improved_bank WHERE owner_account = {}", accountId);
    }
};

void AddSC_improved_bank_account_script()
{
    new improved_bank_account_script();
}
