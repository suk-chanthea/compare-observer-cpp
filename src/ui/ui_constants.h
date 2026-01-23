#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <QString>

namespace UIConstants {
    // Layout spacing
    constexpr int MAIN_LAYOUT_SPACING = 16;
    constexpr int INFO_LAYOUT_SPACING = 30;
    constexpr int CHECKBOX_SPACING = 12;
    constexpr int PANEL_SPACING = 24;
    
    // Colors
    namespace Colors {
        const QString TEXT_PRIMARY = "#DADADA";
        const QString TEXT_SECONDARY = "#888888";
        const QString TEXT_BRIGHT = "#FFFFFF";
        const QString TEXT_DISABLED = "#555555";
        const QString TEXT_HEADER = "#F5F5F5";
        const QString TEXT_LABEL = "#B5B5B5";
        const QString TEXT_STATUS = "#D5D5D5";
        
        const QString BG_DARK = "#252525";
        const QString BG_DARKER = "#1A1A1A";
        const QString BG_LIGHT = "#2A2A2A";
        
        const QString BORDER_DEFAULT = "#3A3A3A";
        const QString BORDER_HOVER = "#4A4A4A";
        const QString BORDER_ACTIVE = "#5A5A5A";
        const QString BORDER_SEPARATOR = "#353535";
        
        const QString ACCENT_BLUE = "#4A9EFF";
        const QString ACCENT_IDLE = "#888888";
    }
    
    // Font sizes
    constexpr int FONT_SIZE_HEADER = 16;
    constexpr int FONT_SIZE_NORMAL = 13;
    constexpr int FONT_SIZE_SMALL = 12;
    
    // Window geometry
    constexpr int WINDOW_WIDTH = 1200;
    constexpr int WINDOW_HEIGHT = 700;
}

#endif // UI_CONSTANTS_H

