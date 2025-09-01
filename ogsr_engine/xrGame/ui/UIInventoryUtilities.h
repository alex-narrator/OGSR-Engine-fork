#pragma once

#include "../inventory_item.h"
#include "../character_info_defs.h"

class CUIStatic;

//размеры сетки в текстуре иконок персонажей
#define ICON_GRID_WIDTH 64
#define ICON_GRID_HEIGHT 64
//размер иконки персонажа для инвенторя и торговли
#define CHAR_ICON_WIDTH 2
#define CHAR_ICON_HEIGHT 2

namespace InventoryUtilities
{

// Получить значение времени в текстовом виде

// Точность возвращаемого функцией GetGameDateTimeAsString значения: до часов, до минут, до секунд
enum ETimePrecision
{
    etpTimeToHours = 0,
    etpTimeToMinutes,
    etpTimeToSeconds,
    etpTimeToMilisecs,
    etpTimeToSecondsAndDay
};

// Точность возвращаемого функцией GetGameDateTimeAsString значения: до года, до месяца, до дня
enum EDatePrecision
{
    edpDateToDay,
    edpDateToMonth,
    edpDateToYear
};

const shared_str GetDateAsString(ALife::_TIME_ID time, EDatePrecision datePrec, char dateSeparator = '/');
const shared_str GetTimeAsString(ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator = ':');
LPCSTR GetTimePeriodAsString(LPSTR _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to);

// Функции получения строки-идентификатора ранга и отношения по их числовому идентификатору
LPCSTR GetRankAsText(CHARACTER_RANK_VALUE rankID);
LPCSTR GetReputationAsText(CHARACTER_REPUTATION_VALUE rankID);
LPCSTR GetGoodwillAsText(CHARACTER_GOODWILL goodwill);

void ClearCharacterInfoStrings();

void SendInfoToActor(LPCSTR info_id);
u32 GetGoodwillColor(CHARACTER_GOODWILL gw);
u32 GetRelationColor(ALife::ERelationType r);
u32 GetReputationColor(CHARACTER_REPUTATION_VALUE rv);
}; // namespace InventoryUtilities