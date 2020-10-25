#ifndef NX_SHELL_LANGUAGE_H
#define NX_SHELL_LANGUAGE_H

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
        SettingsImageViewTitle,
        SettingsDevOptsTitle,
        SettingsAboutTitle,
        SettingsCheckForUpdates,
        SettingsSortNameAsc,
        SettingsSortNameDesc,
        SettingsSortSizeLarge,
        SettingsSortSizeSmall,
        SettingsImageViewFilenameToggle,
        SettingsDevOptsLogsToggle,
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

        // Max
        Max
    } StringID;
}

extern const char **strings[Lang::Max];

#endif
