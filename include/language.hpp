#pragma once

namespace Lang {
    typedef enum {
        // Prompt/Message buttons
        ButtonOK = 0,
        ButtonCancel,

        // Options dialog
        OptionsTitle,
        OptionsSelectAll,
        OptionsClearAll,
        OptionsProperties,
        OptionsRename,
        OptionsNewFolder,
        OptionsNewFile,
        OptionsCopy,
        OptionsMove,
        OptionsPaste,
        OptionsDelete,
        OptionsSetArchiveBit,
        OptionsRenamePrompt,
        OptionsFolderPrompt,
        OptionsFilePrompt,
        OptionsCopying,

        // Properties dialog
        PropertiesName,
        PropertiesSize,
        PropertiesCreated,
        PropertiesModified,
        PropertiesAccessed,
        PropertiesWidth,
        PropertiesHeight,

        // Delete dialog
        DeleteMessage,
        DeleteMultiplePrompt,
        DeletePrompt,

        // Archive dialog
        ArchiveTitle,
        ArchiveMessage,
        ArchivePrompt,
        ArchiveExtracting,

        // SettingsWindow
        SettingsTitle,
        SettingsSortTitle,
        SettingsLanguageTitle,
        SettingsUSBTitle,
        SettingsUSBUnmount,
        SettingsImageViewTitle,
        SettingsDevOptsTitle,
        SettingsMultiLangTitle,
        SettingsAboutTitle,
        SettingsCheckForUpdates,
        SettingsImageViewFilenameToggle,
        SettingsDevOptsLogsToggle,
        SettingsMultiLangLogsToggle,
        SettingsAboutVersion,
        SettingsAboutAuthor,
        SettingsAboutBanner,

        // Updates Dialog
        UpdateTitle,
        UpdateNetworkError,
        UpdateAvailable,
        UpdatePrompt,
        UpdateSuccess,
        UpdateRestart,
        UpdateNotAvailable,

        // USB Dialog
        USBUnmountPrompt,
        USBUnmountSuccess,

        // Keyboard
        KeyboardEmpty,

        // Max
        Max
    } StringID;
}

extern const char **strings[Lang::Max];
