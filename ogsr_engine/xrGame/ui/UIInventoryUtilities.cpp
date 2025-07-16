#include "stdafx.h"
#include "UIInventoryUtilities.h"
#include "WeaponAmmo.h"
#include "Weapon.h"
#include "WeaponRPG7.h"
#include "UIStaticItem.h"
#include "UIStatic.h"
#include "eatable_item.h"
#include "Level.h"
#include "HUDManager.h"
#include "UIGameSP.h"
#include "date_time.h"
#include "string_table.h"
#include "Inventory.h"
#include "InventoryOwner.h"
#include "InventoryBox.h"

#include "InfoPortion.h"
#include "game_base_space.h"
#include "actor.h"
#include "string_table.h"
#include "script_game_object.h"

constexpr auto EQUIPMENT_ICONS = "ui\\ui_icon_equipment";

constexpr LPCSTR relationsLtxSection = "game_relations";
constexpr LPCSTR ratingField = "rating_names";
constexpr LPCSTR reputationgField = "reputation_names";
constexpr LPCSTR goodwillField = "goodwill_names";

typedef std::pair<CHARACTER_RANK_VALUE, shared_str> CharInfoStringID;
DEF_MAP(CharInfoStrings, CHARACTER_RANK_VALUE, shared_str);

CharInfoStrings* charInfoReputationStrings = NULL;
CharInfoStrings* charInfoRankStrings = NULL;
CharInfoStrings* charInfoGoodwillStrings = NULL;

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetTimeAsString(ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator)
{
    string64 bufTime{};

    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

    split_time(time, year, month, day, hours, mins, secs, milisecs);

    // Time
    switch (timePrec)
    {
    case etpTimeToHours: sprintf_s(bufTime, "%02i", hours); break;
    case etpTimeToMinutes: sprintf_s(bufTime, "%02i%c%02i", hours, timeSeparator, mins); break;
    case etpTimeToSeconds: sprintf_s(bufTime, "%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs); break;
    case etpTimeToMilisecs: sprintf_s(bufTime, "%02i%c%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs, timeSeparator, milisecs); break;
    case etpTimeToSecondsAndDay: {
        int total_day = (int)(time / (1000 * 60 * 60 * 24));
        sprintf_s(bufTime, sizeof(bufTime), "%dd %02i%c%02i%c%02i", total_day, hours, timeSeparator, mins, timeSeparator, secs);
        break;
    }
    default: R_ASSERT(!"Unknown type of date precision");
    }

    return bufTime;
}

const shared_str InventoryUtilities::GetDateAsString(ALife::_TIME_ID date, EDatePrecision datePrec, char dateSeparator)
{
    string32 bufDate{};

    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

    split_time(date, year, month, day, hours, mins, secs, milisecs);

    // Date
    switch (datePrec)
    {
    case edpDateToYear: sprintf_s(bufDate, "%04i", year); break;
    case edpDateToMonth: sprintf_s(bufDate, "%02i%c%04i", month, dateSeparator, year); break;
    case edpDateToDay: sprintf_s(bufDate, "%02i%c%02i%c%04i", day, dateSeparator, month, dateSeparator, year); break;
    default: R_ASSERT(!"Unknown type of date precision");
    }

    return bufDate;
}

LPCSTR InventoryUtilities::GetTimePeriodAsString(LPSTR _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to)
{
    u32 year1, month1, day1, hours1, mins1, secs1, milisecs1;
    u32 year2, month2, day2, hours2, mins2, secs2, milisecs2;

    split_time(_from, year1, month1, day1, hours1, mins1, secs1, milisecs1);
    split_time(_to, year2, month2, day2, hours2, mins2, secs2, milisecs2);

    int cnt = 0;
    _buff[0] = 0;

    if (month1 != month2)
        cnt = sprintf_s(_buff + cnt, buff_sz - cnt, "%d %s ", month2 - month1, *CStringTable().translate("ui_st_months"));

    if (!cnt && day1 != day2)
        cnt = sprintf_s(_buff + cnt, buff_sz - cnt, "%d %s", day2 - day1, *CStringTable().translate("ui_st_days"));

    if (!cnt && hours1 != hours2)
        cnt = sprintf_s(_buff + cnt, buff_sz - cnt, "%d %s", hours2 - hours1, *CStringTable().translate("ui_st_hours"));

    if (!cnt && mins1 != mins2)
        cnt = sprintf_s(_buff + cnt, buff_sz - cnt, "%d %s", mins2 - mins1, *CStringTable().translate("ui_st_mins"));

    if (!cnt && secs1 != secs2)
        cnt = sprintf_s(_buff + cnt, buff_sz - cnt, "%d %s", secs2 - secs1, *CStringTable().translate("ui_st_secs"));

    return _buff;
}

//////////////////////////////////////////////////////////////////////////
void LoadStrings(CharInfoStrings* container, LPCSTR section, LPCSTR field)
{
    R_ASSERT(container);

    LPCSTR cfgRecord = pSettings->r_string(section, field);
    u32 count = _GetItemCount(cfgRecord);
    R_ASSERT3(count % 2, "there're must be an odd number of elements", field);
    string64 singleThreshold{};
    int upBoundThreshold = 0;
    CharInfoStringID id;

    for (u32 k = 0; k < count; k += 2)
    {
        _GetItem(cfgRecord, k, singleThreshold);
        id.second = singleThreshold;

        _GetItem(cfgRecord, k + 1, singleThreshold);
        if (k + 1 != count)
            sscanf(singleThreshold, "%i", &upBoundThreshold);
        else
            upBoundThreshold += 1;

        id.first = upBoundThreshold;

        container->insert(id);
    }
}

//////////////////////////////////////////////////////////////////////////

void InitCharacterInfoStrings()
{
    if (charInfoReputationStrings && charInfoRankStrings)
        return;

    if (!charInfoReputationStrings)
    {
        // Create string->Id DB
        charInfoReputationStrings = xr_new<CharInfoStrings>();
        // Reputation
        LoadStrings(charInfoReputationStrings, relationsLtxSection, reputationgField);
    }

    if (!charInfoRankStrings)
    {
        // Create string->Id DB
        charInfoRankStrings = xr_new<CharInfoStrings>();
        // Ranks
        LoadStrings(charInfoRankStrings, relationsLtxSection, ratingField);
    }

    if (!charInfoGoodwillStrings)
    {
        // Create string->Id DB
        charInfoGoodwillStrings = xr_new<CharInfoStrings>();
        // Goodwills
        LoadStrings(charInfoGoodwillStrings, relationsLtxSection, goodwillField);
    }
}

//////////////////////////////////////////////////////////////////////////

void InventoryUtilities::ClearCharacterInfoStrings()
{
    xr_delete(charInfoReputationStrings);
    xr_delete(charInfoRankStrings);
    xr_delete(charInfoGoodwillStrings);
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetRankAsText(CHARACTER_RANK_VALUE rankID)
{
    InitCharacterInfoStrings();
    CharInfoStrings::const_iterator cit = charInfoRankStrings->upper_bound(rankID);
    if (charInfoRankStrings->end() == cit)
        return charInfoRankStrings->rbegin()->second.c_str();
    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetReputationAsText(CHARACTER_REPUTATION_VALUE rankID)
{
    InitCharacterInfoStrings();

    CharInfoStrings::const_iterator cit = charInfoReputationStrings->upper_bound(rankID);
    if (charInfoReputationStrings->end() == cit)
        return charInfoReputationStrings->rbegin()->second.c_str();

    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetGoodwillAsText(CHARACTER_GOODWILL goodwill)
{
    InitCharacterInfoStrings();

    CharInfoStrings::const_iterator cit = charInfoGoodwillStrings->upper_bound(goodwill);
    if (charInfoGoodwillStrings->end() == cit)
        return charInfoGoodwillStrings->rbegin()->second.c_str();

    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////
// специальная функция для передачи info_portions при нажатии кнопок UI
// (для tutorial)
void InventoryUtilities::SendInfoToActor(LPCSTR info_id)
{
    CActor* actor = smart_cast<CActor*>(Level().CurrentEntity());
    if (actor)
    {
        actor->TransferInfo(info_id, true);
    }
}

u32 InventoryUtilities::GetGoodwillColor(CHARACTER_GOODWILL gw)
{
    if (gw >= charInfoGoodwillStrings->rbegin()->first)
        return 0xff00ff00;
    if (gw < charInfoGoodwillStrings->begin()->first)
        return 0xffff0000;
    return 0xffc0c0c0;
}

u32 InventoryUtilities::GetReputationColor(CHARACTER_REPUTATION_VALUE rv)
{
    if (rv >= charInfoReputationStrings->rbegin()->first)
        return 0xff00ff00;
    if (rv < charInfoReputationStrings->begin()->first)
        return 0xffff0000;
    return 0xffc0c0c0;
}

u32 InventoryUtilities::GetRelationColor(ALife::ERelationType relation)
{
    switch (relation)
    {
    case ALife::eRelationTypeFriend: return 0xff00ff00; break;
    case ALife::eRelationTypeNeutral: return 0xffc0c0c0; break;
    case ALife::eRelationTypeEnemy: return 0xffff0000; break;
    default: NODEFAULT;
    }
#ifdef DEBUG
    return 0xffffffff;
#endif
}