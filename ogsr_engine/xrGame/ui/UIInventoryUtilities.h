#pragma once

#include "../inventory_item.h"
#include "../character_info_defs.h"

class CUIStatic;
class CGameObject;

// размеры сетки в текстуре инвентаря
constexpr auto INV_GRID_WIDTH{50};
constexpr auto INV_GRID_HEIGHT{50};

// размеры сетки в текстуре иконок персонажей
constexpr auto ICON_GRID_WIDTH{64};
constexpr auto ICON_GRID_HEIGHT{64};
// размер иконки персонажа для инвенторя и торговли
constexpr auto CHAR_ICON_WIDTH{2};
constexpr auto CHAR_ICON_HEIGHT{2};

// размер иконки персонажа в полный рост
constexpr auto CHAR_ICON_FULL_WIDTH{2};
constexpr auto CHAR_ICON_FULL_HEIGHT{5};

constexpr auto TRADE_ICONS_SCALE{4.f / 5.f};

namespace InventoryUtilities
{

// для проверки свободного места
bool FreeRoom(TIItemContainer& item_list, PIItem item, int width, int height, bool vertical);
bool FreeRoom_byRows(TIItemContainer& item_list, PIItem item, int width, int height);
// теж саме що й FreeRoom_byRows тільки для предметів з іконками height > width
bool FreeRoom_byColumns(TIItemContainer& item_list, PIItem item, int width, int height);
bool HasFreeSpace(TIItemContainer&, PIItem, int, int, bool);

// получить shader на иконки инвенторя
ui_shader& GetEquipmentIconsShader(shared_str icon_group);
// удаляем все шейдеры
void DestroyShaders();
void CreateShaders();

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

const shared_str GetGameDateAsString(EDatePrecision datePrec, char dateSeparator = '/');
const shared_str GetGameTimeAsString(ETimePrecision timePrec, char timeSeparator = ':');
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