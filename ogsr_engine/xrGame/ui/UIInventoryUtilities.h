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

// сравнивает элементы по пространству занимаемому ими в рюкзаке
// для сортировки
bool GreaterRoomInRuck(PIItem item1, PIItem item2);
// для проверки свободного места
bool FreeRoom_inBelt(TIItemContainer& item_list, PIItem item, int width, int height);
// теж саме що й FreeRoom_inBelt тільки для предметів з іконками height > width
bool FreeRoom_inVest(TIItemContainer& item_list, PIItem item, int width, int height);
bool HasFreeSpace(TIItemContainer&, PIItem, int, int);

// получить shader на иконки инвенторя
ui_shader& GetEquipmentIconsShader(size_t icon_group = 0);
// удаляем все шейдеры
void DestroyShaders();
void CreateShaders();

//приєднання додаткових іконок
void TryAttachIcons(CUIStatic*, PIItem, float = 1.f);

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
// Отобразить вес, который несет актер
void UpdateWeight(CUIStatic& wnd, bool withPrefix = false);

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