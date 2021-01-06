#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
#Warn   ; Enable warnings to assist with detecting common errors.
#SingleInstance Force
#InstallKeybdHook
#InstallMouseHook
#KeyHistory, 0
#HotKeyInterval 1
#MaxHotkeysPerInterval 127
#UseHook
#Persistent

SetKeyDelay, -1, 8
SetControlDelay, -1
SetWinDelay, -1
SendMode, InputThenPlay
SetBatchLines,-1
ListLines, Off
CoordMode, Pixel, Window

#Include %A_ScriptDir%
#Include Lib.ahk
#Include Character.ahk
#Include Bot.ahk

global opts := new Options(Func("OptChanged"))
global bot := new AutoBot(opts)

Menu, Tray, Add, For Honor Bot, ButtonShowGui
Menu, Tray, Default, For Honor Bot

Gui 1:Font, s11
Gui 2:Font, s11
Gui Add, Text, x10 y10, Bot:
Gui Add, Text, x+4 y10 vBotStatus, Disabled

Gui Add, Button, x200 y10, Settings

Gui Add, Text, x10 y+10, Default options:
opts.Add("OptBlock",            "CheckBox", "Auto block").Init("checked")
opts.Add("OptGuardBrake",       "CheckBox", "Auto guard brake").Init("checked")
opts.Add("OptParry",            "CheckBox", "Auto parry").Init("checked")
opts.Add("OptEvade",            "CheckBox", "Auto evade bashes").Init("checked")
opts.Add("OptDeflect",          "CheckBox", "Auto deflect").Init("checked")
opts.Add("OptBlockOther",       "CheckBox", "Auto block (other)").Init()
opts.Add("OptParryOther",       "CheckBox", "Auto parry (other)").Init()
opts.Add("OptAttackUnblock",    "CheckBox", "Attack unblockable").Init()
opts.Add("OptAttackEvade",      "CheckBox", "Attack on evade").Init()
opts.Add("OptCheckBlock",       "CheckBox", "Check block on parry").Init("checked")

Gui Add, Text, x200 y50, Debug options:
opts.Add("OptVisibleArea",      "CheckBox", "Visible Area").Init()
opts.Add("OptVisiblePoints",    "CheckBox", "Visible Points").Init()
opts.Add("OptDisableActions",   "CheckBox", "Disable Actions").Init()
opts.Add("OptScreenshots",      "CheckBox", "Screenshots").Init()

Gui Add, Text, y+10, Evade time:
Gui Add, Edit
opts.Add("OptEvadeTime",        "UpDown",   "50").Init("Range0-200")

Gui Add, Text, y+10, Special character:
opts.Add("OptChar",             "DropDownList", CharRegistry.AsList()).Init("Choose1")

Gui 2:Add, Text, x10, Keybindings:
AddKeybindSetting("KbAttackAOE",    "Attack AOE", "q", bot.AttackAOE.Bind(bot))
AddKeybindSetting("KbAction",       "Action", "f", bot.KeyAction.Bind(bot))
AddKeybindSettingOption("OptBlock")
AddKeybindSettingOption("OptGuardBrake")
AddKeybindSettingOption("OptParry")
AddKeybindSettingOption("OptEvade")
AddKeybindSettingOption("OptDeflect")
AddKeybindSettingOption("OptBlockOther")
AddKeybindSettingOption("OptParryOther")
AddKeybindSettingOption("OptAttackUnblock")
AddKeybindSettingOption("OptAttackEvade")

IniStorage.global.Load()

Gui Show,, For Honor Bot
Gui Submit, NoHide

return

AddKeybindSetting(name, text, default, callabck="") {
    opts.AddControl(new WrappedHotkey(name, callabck)).Init("x10 h25 w75", 2)
    if default
        opts[name].Set(default)
    Gui 2:Add, Text, x+5 h25 0x200, % text
}

AddKeybindSettingOption(optName) {
    opt := opts[optName]
    name := optName "KB"
    AddKeybindSetting(name, "Switch '" opt.text "'", "", Func("OptSwitch").bind(opt))
}

OptChanged(opt) {
    bot.MarkDirty()
}

OptSwitch(opt) {
    opt.Switch()
    bot.MarkDirty()
}

ButtonEnableSwitch(enable) {
    if bot.enabled = enable {
        return
    }

    if enable {
        GuiControl, +c00AA00, BotStatus
        GuiControl,, BotStatus, Enabled
        bot.Enable()
    } else {
        GuiControl, +cFF0000, BotStatus
        GuiControl,, BotStatus, Disabled
        bot.Disable()
    }
}

$F1::
ButtonEnableSwitch(1)
return

$F2::
ButtonEnableSwitch(0)
return

ButtonHideGui:
Gui Cancel
return

ButtonShowGui:
Gui Show
return

ButtonSettings:
Gui 2:Show,, Settings
return

~MButton::
bot.CooldownGB()
return

~RButton::
bot.CooldownGB()
return
