#pragma once

namespace GameObject
{
enum ECallbackType
{
    eZoneEnter = u32(0),
    eZoneExit,
    eExitLevelBorder,
    eEnterLevelBorder,
    eDeath,

    ePatrolPathInPoint,

    //		eInventoryPda,
    eInventoryInfo,
    eArticleInfo,
    eTaskStateChange,
    eMapLocationAdded,
    eMapLocationClicked,

    eUseObject,

    eHit,

    eSound,

    eActionTypeMovement,
    eActionTypeWatch,
    eActionTypeAnimation,
    eActionTypeSound,
    eActionTypeParticle,
    eActionTypeObject,

    //		eActorSleep,

    eHelicopterOnPoint,
    eHelicopterOnHit,

    eOnItemTake,
    eOnItemDrop,

    eScriptAnimation,

    eTraderGlobalAnimationRequest,
    eTraderHeadAnimationRequest,
    eTraderSoundEnd,

    eInvBoxItemTake,
    eInvBoxItemPlace,

    eOnKeyPress,
    eOnKeyRelease,
    eOnKeyHold,
    eOnMouseWheel,
    eOnMouseMove,
    eOnItemToBelt,
    eOnItemToRuck,
    eOnItemToSlot,
    eOnBeforeUseItem,
    entity_alive_before_hit,

    eOnUpdateAddonsVisibiility,
    eOnUpdateHUDAddonsVisibiility,
    eOnAddonInit,

    eOnHudStateSwitch,
    eOnMotionMark,

    // Called when the player zooms their weapon in or out.
    eOnActorWeaponZoomIn,
    eOnActorWeaponZoomOut,
    eOnActorWeaponZoomChange,
    eOnActorWeaponScopeModeChange,
    eOnActorWeaponFireModeChange,
    eOnActorWeaponGrenadeModeChange,

    eBeforeSave,
    ePostSave,

    eUIMapClick,
    eUIMapDbClick,

    eOnWpnShellDrop,
    eOnThrowGrenade,
    eOnGoodwillChange,
    eLevelChangerAction,

    eOnUpdateItemsEffect,

    eAttachVehicle,
    eDetachVehicle,
    eUseVehicle,

    eOnInvBoxItemTake,
    eOnInvBoxItemDrop,
    eOnInvBoxOpen,

    eSelectPdaContact,

    eOnActorFootStep,
    eOnActorLand,
    eOnActorJump,
    eOnActorBoltThrow,

    eOnPDAContactAdd,
    eOnPDAContactRemove,

    eDummy = u32(-1),
};
};
